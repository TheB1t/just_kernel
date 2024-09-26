#include <drivers/char/tty.h>
#include <fs/devfs.h>
#include <proc/sched.h>

tty_t* tty_table[MAX_TTYS]       = { 0 };
tty_ldisc_t ldiscs[NR_LDISCS]    = { 0 };
int32_t fg_console               = 0;

int32_t tty_register_ldisc(int32_t disc, tty_ldisc_t* new_ldisc) {
	if (disc >= NR_LDISCS)
		return -EINVAL;

	if (new_ldisc) {
		ldiscs[disc] = *new_ldisc;
	} else
		memset((uint8_t*)&ldiscs[disc], 0, sizeof(struct tty_ldisc));

	return 0;
}

void put_tty_queue(char c, tty_queue_t* queue) {
	int32_t head = (queue->head + 1) & (TTY_BUF_SIZE - 1);
	if (head != queue->tail) {
		queue->buf[queue->head] = c;
		queue->head = head;
	}
}

int32_t get_tty_queue(tty_queue_t* queue) {
	int32_t result = -1;

	if (queue->tail != queue->head) {
		result = 0xff & queue->buf[queue->tail];
		queue->tail = (queue->tail + 1) & (TTY_BUF_SIZE - 1);
	}

	return result;
}

static void initialize_tty_struct(uint32_t line, tty_t* tty) {
    memset((uint8_t*)tty, 0, sizeof(tty_t));

    tty->disc = 0;
    tty->line = line;

	if (IS_A_CONSOLE(line)) {
		tty->open = console_open;
	} else if IS_A_SERIAL(line) {
		tty->open = NULL;
	} else if IS_A_PTY(line) {
		tty->open = pty_open;
	}
}

static int32_t init_dev(uint32_t dev) {
    int32_t o_dev = PTY_OTHER(dev);
    tty_t* tty = NULL;
    tty_t* o_tty = NULL;
    int32_t retval = 0;

repeat:
    retval = -EAGAIN;
    if (IS_A_PTY_MASTER(dev) && TTY_TABLE(dev) && TTY_TABLE(dev)->count)
        goto end_init;

    retval = -ENOMEM;

    if (!TTY_TABLE(dev) && !tty) {
        tty = (tty_t*)kmalloc(sizeof(tty_t));
        if (!tty)
            goto end_init;
        initialize_tty_struct(dev, tty);
        goto repeat;
    }

    if (IS_A_PTY(dev)) {
        if (!TTY_TABLE(o_dev) && !o_tty) {
            o_tty = (tty_t*)kmalloc(sizeof(tty_t));
            if (!o_tty)
                goto end_init;
            initialize_tty_struct(o_dev, o_tty);
            goto repeat;
        }
    }

    if (!TTY_TABLE(dev)) {
        TTY_TABLE(dev) = tty;
        tty = NULL;
    }

    if (IS_A_PTY(dev)) {
        if (!TTY_TABLE(o_dev)) {
            TTY_TABLE(o_dev) = o_tty;
            o_tty = NULL;
        }

        TTY_TABLE(dev)->link = TTY_TABLE(o_dev);
        TTY_TABLE(o_dev)->link = TTY_TABLE(dev);
    }

    TTY_TABLE(dev)->count++;
    if (IS_A_PTY_MASTER(dev))
        TTY_TABLE(o_dev)->count++;

    retval = 0;

end_init:
    if (tty)
        kfree(tty);

    if (o_tty)
        kfree(o_tty);

    return retval;
}

static void release_dev(int32_t dev, file_t* file) {
    if (!TTY_TABLE(dev))
        return;

    tty_t* tty = TTY_TABLE(dev);
    tty_t* o_tty = NULL;

	if (IS_A_PTY(dev)) {
		o_tty = TTY_TABLE(PTY_OTHER(dev));

		if (tty->link != o_tty || o_tty->link != tty)
			return;
	}

    if (tty->close)
        tty->close(tty, file);

    if (IS_A_PTY_MASTER(dev)) {
        if (--tty->link->count < 0)
            tty->link->count = 0;
    }

    if (--tty->count < 0)
        tty->count = 0;

    if (tty->count)
        return;

    // TODO: Terminate all processes on this tty

	if (o_tty) {
		if (o_tty->count)
			return;
		else
			TTY_TABLE(PTY_OTHER(dev)) = NULL;
	}
    TTY_TABLE(dev) = NULL;

	kfree((void*)tty);
	if (o_tty)
		kfree((void*)o_tty);
}

int32_t tty_open(inode_t* inode, file_t* file) {
    uint8_t major = MAJOR(inode->i_rdev);
    uint8_t minor = MINOR(inode->i_rdev);

    if (major == TTYAUX_MAJOR) {
        if (!minor) {
            major = TTY_MAJOR;
            minor = current_proc->tty;
        }
    } else if (major == TTY_MAJOR) {
		if (!minor)
			minor = fg_console + 1;
    } else {
        return -ENODEV;
    }

    file->f_rdev = MKDEV(major, minor);

    int32_t retval = init_dev(minor);
    if (retval)
        return retval;

    tty_t* tty = TTY_TABLE(minor);

    if (tty->open)
        retval = tty->open(tty, file);
    else
        retval = -ENODEV;

    if (retval) {
        release_dev(minor, file);
        return retval;
    }

    file->f_rdev = 0x4000 | minor;

    return 0;
}

int32_t tty_read(inode_t* inode, file_t* file, char* buf, uint32_t size) {
    uint32_t major = MAJOR(inode->i_rdev);
    uint32_t minor = MINOR(inode->i_rdev);

    if (major != TTY_MAJOR)
        return -EINVAL;

    tty_t* tty = TTY_TABLE(minor);

    if (tty) {
        if (ldiscs[tty->disc].read)
            return (ldiscs[tty->disc].read)(tty, file, buf, size);
    }

	return -EIO;
}

int32_t tty_write(inode_t* inode, file_t* file, char* buf, uint32_t size) {
    uint32_t major = MAJOR(inode->i_rdev);
    uint32_t minor = MINOR(inode->i_rdev);

    if (major != TTY_MAJOR)
        return -EINVAL;

    tty_t* tty = TTY_TABLE(minor);

    if (tty) {
        if (ldiscs[tty->disc].write)
            return (ldiscs[tty->disc].write)(tty, file, buf, size);
    }

	return -EIO;
}

void tty_write_flush(tty_t* tty) {
	if (!tty->write || EMPTY(&tty->write_q))
		return;

	tty->write(tty);
}

void tty_read_flush(tty_t* tty) {
	if (!tty || EMPTY(&tty->read_q))
		return;

	ldiscs[tty->disc].handler(tty);
}


file_ops_t tty_ops = {
    .lseek      = NULL,
    .read       = tty_read,
    .write      = tty_write,
    .readdir    = NULL,
    .open       = tty_open,
    .release    = NULL,
};

static int32_t write_chan(tty_t* tty, file_t* file UNUSED, char* buf, int32_t nr) {
	char c, *b = buf;

	if (nr < 0)
		return -EINVAL;

	if (!nr)
		return 0;

	while (nr > 0) {
		while (nr > 0 && !FULL(&tty->write_q)) {
			c = *b;

			switch (c) {
				case '\n':
					tty->column = 0;
				    break;

				case '\r':
					tty->column = 0;
					break;

				case '\t':
					c = ' ';
					tty->column++;
					if (tty->column % 8 != 0) {
						b--; nr++;
					}
		            break;

				case '\b':
					tty->column--;
					break;

                default:
					tty->column++;
					break;
			}

			b++; nr--;
			put_tty_queue(c, &tty->write_q);
		}
	}

	TTY_WRITE_FLUSH(tty);
	if (b - buf)
		return b - buf;

	if (tty->link && !tty->link->count)
		return -EPIPE;

	return 0;
}

// static int32_t read_chan(struct tty_struct * tty, struct file * file, char * buf, int32_t nr) {

// }

static tty_ldisc_t tty_ldisc_N_TTY = {
	.open       = NULL,
    .close      = NULL,
    .read       = NULL,
    .write      = write_chan,
    .handler    = NULL,
};

void tty_init() {
    if (devfs_register_device(TTY_MAJOR, "tty", DEVFS_DEVICE_CHAR, &tty_ops))
        panic("Failed to register tty device");

    if (devfs_register_device(TTYAUX_MAJOR, "tty", DEVFS_DEVICE_CHAR, &tty_ops))
        panic("Failed to register ttyaux device");

    tty_register_ldisc(0, &tty_ldisc_N_TTY);
}

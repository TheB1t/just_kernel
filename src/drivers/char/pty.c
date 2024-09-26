#include <drivers/char/tty.h>

static void pty_close(tty_t* tty, file_t* file UNUSED) {
	if (!tty)
		return;

	if (IS_A_PTY_MASTER(tty->line)) {
		if (tty->count > 1)
			return;
	} else {
		if (tty->count > 2)
			return;
	}

	if (!tty->link)
		return;

	if (IS_A_PTY_MASTER(tty->line)) {
		// TODO: Implement this
	}
}

static inline void pty_copy(tty_t* from, tty_t* to) {
	int32_t c;

	while (!EMPTY(&from->write_q)) {
		if (FULL(&to->read_q)) {
			TTY_READ_FLUSH(to);
			if (FULL(&to->read_q))
				break;
			continue;
		}
		c = get_tty_queue(&from->write_q);
		put_tty_queue(c, &to->read_q);
	}
	TTY_READ_FLUSH(to);
}

static void pty_write(tty_t* tty) {
	if (tty->link)
		pty_copy(tty, tty->link);
}

int32_t pty_open(tty_t* tty, file_t* file UNUSED) {
	if (!tty || !tty->link)
		return -ENODEV;

    tty->write = tty->link->write = pty_write;
	tty->close = tty->link->close = pty_close;

    return 0;
}
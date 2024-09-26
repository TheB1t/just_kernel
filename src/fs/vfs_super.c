#include <fs/vfs.h>
#include <fs/devfs.h>
#include <proc/sched.h>

#define BITS_PER_ENTRY (8 * sizeof(uint32_t))

static uint32_t unnamed_dev_in_use[256 / BITS_PER_ENTRY] = { 0 };

dev_t get_unnamed_dev(void) {
	for (int32_t i = 1; i < 256; i++) {
        uint32_t idx = i / BITS_PER_ENTRY;
        uint32_t bit  = i % BITS_PER_ENTRY;

		if (unnamed_dev_in_use[idx] & (1 << bit))
			return MKDEV(UNNAMED_MAJOR, i);
	}

	return MKDEV(0, 0);
}

void put_unnamed_dev(dev_t dev) {
	if (!dev)
		return;

    uint32_t idx = MINOR(dev) / BITS_PER_ENTRY;
    uint32_t bit  = MINOR(dev) % BITS_PER_ENTRY;

    if (MAJOR(dev) != UNNAMED_MAJOR || unnamed_dev_in_use[idx] & (1 << bit)) {
		ser_printf("Cant put unnamed dev %d\n", dev);
		return;
	}

    unnamed_dev_in_use[idx] &= ~(1 << bit);
}


super_block_t super_blocks[NR_SUPER] = { 0 };

static super_block_t* get_super(dev_t dev) {
	super_block_t* s = super_blocks;

	if (!dev)
		return NULL;

	while (s < NR_SUPER + super_blocks) {
		if (s->s_dev == dev) {
			return s;
		} else
			s++;
    }

	return NULL;
}

void put_super(dev_t dev) {
	super_block_t* sb = NULL;

	if (!(sb = get_super(dev)))
		return;

	if (sb->s_covered)
		return;

	if (CHECK_SOP(sb, put_super))
		sb->s_op->put_super(sb);
}

static super_block_t* read_super(dev_t dev, char* name, int32_t flags, void* data, int32_t silent) {
    super_block_t*  s       = NULL;
    filesystem_t*   type    = NULL;

	if (!dev)
		return NULL;

	s = get_super(dev);
	if (s)
		return s;

    type = vfs_find_filesystem(name);
	if (!type)
		return NULL;

	for (s = super_blocks ;; s++) {
		if (s >= NR_SUPER + super_blocks)
			return NULL;

		if (!s->s_dev)
			break;
	}

	s->s_dev = dev;
	s->s_flags = flags;

	if (!type->read_super(s, data, silent)) {
		s->s_dev = 0;
		return NULL;
	}

	s->s_dev = dev;
	s->s_covered = NULL;
	s->s_type = type;

	return s;
}


int32_t do_mount(dev_t dev, char* dev_name, char* dir_name, char* type, uint32_t flags, void* data) {
	inode_t*        dir_i   = NULL;
    super_block_t*  sb      = NULL;
    vfsmount_t*     vfsmnt  = NULL;
    int32_t         error   = 0;

	error = namei(dir_name, &dir_i);
	if (error)
		return error;

	if (dir_i->i_count != 1 || dir_i->i_mount) {
		iput(dir_i);
		return -EBUSY;
	}

	if (!S_ISDIR(dir_i->i_mode)) {
		iput(dir_i);
		return -ENOTDIR;
	}

	sb = read_super(dev, type, flags, data, 0);
	if (!sb) {
		iput(dir_i);
		return -EINVAL;
	}

	if (sb->s_covered) {
		iput(dir_i);
		return -EBUSY;
	}

	vfsmnt = add_vfsmnt(dev, dev_name, dir_name);
	if (vfsmnt) {
		vfsmnt->mnt_sb = sb;
		vfsmnt->mnt_flags = flags;
	}

	sb->s_covered = dir_i;
	dir_i->i_mount = sb->s_mounted;

	return 0;
}

int32_t vfs_mount(char* dev_name, char* dir_name, char* type, uint32_t new_flags, void* data) {
    char*           t       = NULL;
	file_ops_t*     fops    = NULL;
    inode_t*        inode   = NULL;
    filesystem_t*   fstype  = NULL;
    dev_t           dev     = { 0 };
	int32_t         retval  = 0;
	uint32_t        flags   = 0;

	fstype = vfs_find_filesystem(type);
	if (!fstype)
		return -ENODEV;

	t = fstype->name;
	if (fstype->requires_dev) {
		retval = namei(dev_name, &inode);
		if (retval)
			return retval;

		if (!S_ISBLK(inode->i_mode)) {
			iput(inode);
			return -ENOTBLK;
		}

		if (IS_NODEV(inode)) {
			iput(inode);
			return -EACCES;
		}

		dev = inode->i_rdev;
		if (MAJOR(dev) >= MAX_DEVICES) {
			iput(inode);
			return -ENXIO;
		}

		fops = devfs_get_device_ops(MAJOR(dev));
		if (!fops) {
			iput(inode);
			return -ENOTBLK;
		}

		if (fops->open) {
			file_t dummy;	/* allows read-write or read-only flag */
			memset((uint8_t*)&dummy, 0, sizeof(dummy));

			dummy.f_inode = inode;
			dummy.f_mode = TEST_MASK(new_flags, MS_RDONLY) ? MS_RDONLY : 3;

			retval = fops->open(inode, &dummy);
			if (retval) {
				iput(inode);
				return retval;
			}
		}

	} else {
		if (!(dev = get_unnamed_dev()))
			return -EMFILE;

		inode = NULL;
	}

	retval = do_mount(dev, dev_name, dir_name, t, flags, data);

	if (retval && !fstype->requires_dev)
		put_unnamed_dev(dev);

	if (retval && fops && fops->release)
		fops->release(inode, NULL);

	iput(inode);

	return retval;
}

void mount_root(void* data) {
	filesystem_t*   fs_type = NULL;
	super_block_t*  sb      = NULL;
	inode_t*        inode   = NULL;
    vfsmount_t*     vfsmnt  = NULL;
	dev_t           dev     = MKDEV(MEM_MAJOR, 0);

	memset((uint8_t*)super_blocks, 0, sizeof(super_blocks));

	for (fs_type = filesystems; fs_type->read_super; fs_type++) {
		if (!fs_type->requires_dev)
			continue;

		sb = read_super(dev, fs_type->name, MS_RDONLY, data, 1);
		if (sb) {
			inode = sb->s_mounted;
			inode->i_count += 3;
			sb->s_covered = inode;

			current_proc->pwd = inode;
			current_proc->root = inode;

            vfsmnt = add_vfsmnt(dev, "/dev/root", "/");

            vfsmnt->mnt_sb = sb;
            vfsmnt->mnt_flags = 0;

			ser_printf("VFS: Mounted root (%s filesystem)%s.\n", fs_type->name,	TEST_MASK(sb->s_flags, MS_RDONLY) ? " readonly" : "");
			return;
		}
	}

	panic("VFS: Unable to mount root");
}
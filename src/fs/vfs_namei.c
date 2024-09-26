#include <drivers/serial.h>
#include <fs/vfs.h>
#include <proc/sched.h>

int32_t lookup(inode_t* dir, char* name, int32_t len, inode_t** result) {
	super_block_t* sb = NULL;

	*result = NULL;
	if (!dir)
		return -ENOENT;

	if (len == 2 && name[0] == '.' && name[1] == '.') {
		if (dir == current_proc->root) {
			*result = dir;
			return 0;
		} else if ((sb = dir->i_sb) && (dir == sb->s_mounted)) {
			sb = dir->i_sb;
			iput(dir);

			dir = sb->s_covered;

			if (!dir)
				return -ENOENT;

			dir->i_count++;
		}
	}

	if (!CHECK_IOP(dir, lookup)) {
		iput(dir);
		return -ENOTDIR;
	}

	if (!len) {
		*result = dir;
		return 0;
	}

	return dir->i_op->lookup(dir, name, len, result);
}

static int32_t dir_namei(char* pathname, int* namelen, char** name, inode_t* base, inode_t** res_inode) {
	*res_inode = NULL;

	if (!base) {
		base = current_proc->pwd;
		base->i_count++;
	}

	char c = *pathname;
	if (c == '/') {
		iput(base);
		base = current_proc->root;
		pathname++;
		base->i_count++;
	}

	char* thisname = NULL;
    inode_t* inode = NULL;
    int32_t error = 0;
	int32_t len = 0;
	while (1) {
		thisname = pathname;
		for(len = 0; (c = *(pathname++)) && (c != '/'); len++);
		if (!c)
			break;

		base->i_count++;
		error = lookup(base, thisname, len, &inode);
		if (error) {
			iput(base);
			return error;
		}

		base = inode;
	}

	if (!base)
		return -ENOENT;

	if (!CHECK_IOP(base, lookup)) {
		iput(base);
		return -ENOTDIR;
	}

	*name = thisname;
	*namelen = len;
	*res_inode = base;
	return 0;
}


static int32_t _namei(char* pathname, inode_t* base, inode_t** res_inode) {
    inode_t*    inode       = NULL;
	char*       basename    = NULL;
	int32_t     namelen     = 0;
    int32_t     error       = 0;

	*res_inode = NULL;

	error = dir_namei(pathname, &namelen, &basename, base, &base);
	if (error)
		return error;

	base->i_count++;
	error = lookup(base, basename, namelen, &inode);
	if (error) {
		iput(base);
		return error;
	}

	iput(base);

	*res_inode = inode;
	return 0;
}

int32_t namei(char* pathname, inode_t** res_inode) {
	return _namei(pathname, NULL, res_inode);
}

int32_t open_namei(char* pathname, int32_t flag, int32_t mode, inode_t** res_inode, inode_t* base) {
	inode_t*    dir         = NULL;
    inode_t*    inode       = NULL;
	char*       basename    = NULL;
	int32_t     namelen     = 0;
    int32_t     error       = 0;

	error = dir_namei(pathname, &namelen, &basename, base, &dir);
	if (error)
		return error;

	if (!namelen) {
		if (TEST_MASK(flag, O_RDWR)) {
			iput(dir);
			return -EISDIR;
		}

		*res_inode = dir;
		return 0;
	}

	dir->i_count++;
	error = lookup(dir, basename, namelen, &inode);
	if (error) {
		if (!TEST_MASK(flag, O_CREAT)) {
			iput(dir);
			return error;
		}

		if (!CHECK_IOP(dir, create)) {
			iput(dir);
			return -EACCES;
		}

		if (IS_RDONLY(dir)) {
			iput(dir);
			return -EROFS;
		}

		return dir->i_op->create(dir, basename, namelen, mode, res_inode);
	}

	if (S_ISDIR(inode->i_mode) && (flag & 2)) {
		iput(inode);
		return -EISDIR;
	}

	if (S_ISBLK(inode->i_mode) || S_ISCHR(inode->i_mode)) {
		if (IS_NODEV(inode)) {
			iput(inode);
			return -EACCES;
		}
	} else {
		if (IS_RDONLY(inode) && (flag & 2)) {
			iput(inode);
			return -EROFS;
		}
	}

	*res_inode = inode;
	return 0;
}

int32_t do_mknod(char* filename, int32_t mode, dev_t dev) {
	inode_t* 	dir			= NULL;
	char* 		basename	= NULL;
	int32_t		namelen		= 0;
	int32_t 	error		= 0;

	error = dir_namei(filename, &namelen, &basename, NULL, &dir);
	if (error)
		return error;

	if (!namelen) {
		iput(dir);
		return -ENOENT;
	}

	if (IS_RDONLY(dir)) {
		iput(dir);
		return -EROFS;
	}

	if (!CHECK_IOP(dir, mknod)) {
		iput(dir);
		return -EPERM;
	}

	return dir->i_op->mknod(dir, basename, namelen, mode, dev);
}

int32_t vfs_mknod(char* filename, int32_t mode, dev_t dev) {
	return do_mknod(filename, mode, dev);
}
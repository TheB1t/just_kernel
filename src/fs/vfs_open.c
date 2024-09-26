#include <drivers/serial.h>
#include <fs/fs.h>
#include <fs/vfs.h>
#include <proc/sched.h>

int32_t vfs_chdir(char* filename) {
	inode_t* inode = NULL;

	int32_t error = namei(filename, &inode);
	if (error)
		return error;

	if (!S_ISDIR(inode->i_mode)) {
		iput(inode);
		return -ENOTDIR;
	}

	iput(current_proc->pwd);
	current_proc->pwd = inode;

	return 0;
}

int32_t vfs_chroot(char* filename) {
	inode_t* inode = NULL;

	int32_t error = namei(filename, &inode);
	if (error)
		return error;

	if (!S_ISDIR(inode->i_mode)) {
		iput(inode);
		return -ENOTDIR;
	}

	iput(current_proc->root);
	current_proc->root = inode;

	return 0;
}

int32_t do_open(char* filename, int32_t flags, int32_t mode) {
	inode_t*    inode   = NULL;
	file_t*     f       = NULL;
	int32_t     flag    = 0;
    int32_t     error   = 0;
    int32_t     fd      = 0;

	for(fd = 0; fd < NR_OPEN; fd++)
		if (!current_proc->files[fd])
			break;

	if (fd >= NR_OPEN)
		return -EMFILE;

	f = (file_t*)kmalloc(sizeof(file_t));
	if (!f)
		return -ENFILE;

	current_proc->files[fd] = f;
	f->f_flags = flag = flags;
	f->f_mode = APPLY_MASK(flag + 1, O_ACCMODE);

	if (f->f_mode)
		flag++;

	if (TEST_MASK(flag, O_TRUNC | O_CREAT))
		SET_MASK(flag, O_RDWR);

	error = open_namei(filename, flag, mode, &inode, NULL);
	if (error) {
		current_proc->files[fd] = NULL;
		return error;
	}

	f->f_inode = inode;
	f->f_pos = 0;

	if (inode->i_op)
		f->f_op = inode->i_op->default_file_ops;
	else
	 	f->f_op = NULL;

	if (CHECK_FOP(f, open)) {
		error = f->f_op->open(inode,f);
		if (error) {
			iput(inode);
			current_proc->files[fd] = NULL;
			return error;
		}
	}

	CLEAR_MASK(inode->i_flags, O_CREAT | O_EXCL | O_NOCTTY | O_TRUNC);
	return fd;
}

int32_t vfs_open(char* filename, int32_t flags, int32_t mode) {
	return do_open(filename, flags, mode);
}

int32_t vfs_creat(char* pathname, int32_t mode) {
	return vfs_open(pathname, O_CREAT | O_WRONLY | O_TRUNC, mode);
}

int32_t close_fp(file_t* file){
	inode_t* inode = file->f_inode;

	if (CHECK_FOP(file, release))
		file->f_op->release(inode, file);

	// file->f_inode = NULL;
	kfree(file);
	iput(inode);

	return 0;
}

int32_t vfs_close(uint32_t fd) {
	if (fd >= NR_OPEN)
		return -EINVAL;

	file_t* file = current_proc->files[fd];
	if (!file)
		return -EINVAL;

	current_proc->files[fd] = NULL;
	return close_fp(file);
}

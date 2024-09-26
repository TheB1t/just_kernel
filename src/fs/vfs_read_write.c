#include <fs/vfs.h>
#include <proc/sched.h>

int32_t vfs_readdir(uint32_t fd, directory_t* dirent, uint32_t count) {
	file_t*		file	= NULL;
	inode_t* 	inode	= NULL;
	int32_t 	error	= 0;

	if (fd >= NR_OPEN || !(file = current_proc->files[fd]) || !(inode = file->f_inode))
		return -EBADF;

	error = -ENOTDIR;
	if (CHECK_FOP(file, readdir))
		error = file->f_op->readdir(inode, file, dirent, count);

	return error;
}

int32_t vfs_lseek(uint32_t fd, uint32_t offset, uint32_t origin) {
	file_t*	file		= NULL;
	int32_t tmp			= -1;

	if (fd >= NR_OPEN || !(file = current_proc->files[fd]) || !(file->f_inode))
		return -EBADF;

	if (origin > 2)
		return -EINVAL;

	if (CHECK_FOP(file, lseek))
		return file->f_op->lseek(file->f_inode, file, offset, origin);

	switch (origin) {
		case 0:
			tmp = offset;
			break;

		case 1:
			tmp = file->f_pos + offset;
			break;

		case 2:
			if (!file->f_inode)
				return -EINVAL;

			tmp = file->f_inode->i_size + offset;
			break;
	}

	if (tmp < 0)
		return -EINVAL;

	ser_printf("tmp: %d\n", tmp);

	file->f_pos = tmp;
	return file->f_pos;
}

int32_t vfs_read(uint32_t fd, char* buf, uint32_t count) {
	file_t* 	file	= NULL;
	inode_t* 	inode	= NULL;

	if (fd >= NR_OPEN || !(file = current_proc->files[fd]) || !(inode = file->f_inode))
		return -EBADF;

	if (!TEST_MASK(file->f_mode, O_WRONLY))
		return -EBADF;

	if (!CHECK_FOP(file, read))
		return -EINVAL;

	if (!count)
		return 0;

	return file->f_op->read(inode, file, buf, count);
}

int32_t vfs_write(uint32_t fd, char* buf, uint32_t count) {
	file_t* 	file	= NULL;
	inode_t* 	inode	= NULL;

	if (fd >= NR_OPEN || !(file = current_proc->files[fd]) || !(inode = file->f_inode))
		return -EBADF;

	if (!TEST_MASK(file->f_mode, O_RDWR))
		return -EBADF;

	if (!CHECK_FOP(file, write))
		return -EINVAL;

	if (!count)
		return 0;

	return file->f_op->write(inode, file, buf, count);
}
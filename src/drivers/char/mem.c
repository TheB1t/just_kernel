#include <drivers/char/mem.h>
#include <fs/devfs.h>

static int32_t read_mem(inode_t* inode UNUSED, file_t* file, char* buf, uint32_t count) {
	unsigned long p = file->f_pos;

	memcpy((uint8_t*)p, (uint8_t*)buf, count);

	file->f_pos += count;

	return count;
}

static int32_t write_mem(inode_t* inode UNUSED, file_t* file, char* buf, uint32_t count) {
	unsigned long p = file->f_pos;

	memcpy((uint8_t*)buf, (uint8_t*)p, count);

	file->f_pos += count;

	return count;
}

static int32_t read_null(inode_t* inode UNUSED, file_t* file UNUSED, char* buf UNUSED, uint32_t count UNUSED) {
	return 0;
}

static int32_t write_null(inode_t* inode UNUSED, file_t* file UNUSED, char* buf UNUSED, uint32_t count UNUSED) {
	return count;
}

static int32_t read_zero(inode_t* inode UNUSED, file_t* file UNUSED, char* buf, uint32_t count) {
	for (int32_t left = count; left > 0; left--) {
		*buf = 0;
		buf++;
	}

	return count;
}

static int32_t null_lseek(inode_t* inode UNUSED, file_t* file, uint32_t offset UNUSED, uint32_t orig UNUSED) {
	return file->f_pos = 0;
}

static int32_t memory_lseek(inode_t* inode UNUSED, file_t* file, uint32_t offset UNUSED, uint32_t orig UNUSED) {
	switch (orig) {
		case 0:
			file->f_pos = offset;
			return file->f_pos;

		case 1:
			file->f_pos += offset;
			return file->f_pos;

		default:
			return -EINVAL;
	}

	if (file->f_pos < 0)
		return 0;

	return file->f_pos;
}

#define write_kmem	write_mem
#define zero_lseek	null_lseek
#define write_zero	write_null

static file_ops_t mem_fops = {
    .lseek      = memory_lseek,
    .read       = read_mem,
    .write      = write_mem,
    .readdir    = NULL,
    .open       = NULL,
    .release    = NULL
};

static file_ops_t null_fops = {
    .lseek      = null_lseek,
    .read       = read_null,
    .write      = write_null,
    .readdir    = NULL,
    .open       = NULL,
    .release    = NULL
};

static file_ops_t zero_fops = {
    .lseek      = zero_lseek,
    .read       = read_zero,
    .write      = write_zero,
    .readdir    = NULL,
    .open       = NULL,
    .release    = NULL
};

static int32_t memory_open(inode_t* inode, file_t* file) {
	switch (MINOR(inode->i_rdev)) {
		case 0:
			file->f_op = &mem_fops;
			break;

		case 1:
			file->f_op = &null_fops;
			break;

		case 2:
			file->f_op = &zero_fops;
			break;

		case 3:
			file->f_op = &null_fops;
			break;

		default:
			return -ENXIO;
	}

	if (CHECK_FOP(file, open))
		return file->f_op->open(inode, file);

	return 0;
}

static file_ops_t memory_fops = {
    .lseek      = NULL,
    .read       = NULL,
    .write      = NULL,
    .readdir    = NULL,
    .open       = memory_open,
    .release    = NULL
};

int32_t mem_init() {
    if (devfs_register_device(MEM_MAJOR, "mem", DEVFS_DEVICE_CHAR, &memory_fops)) {
        ser_printf("Failed to register memory device\n");
    }

	return 0;
}
#include <fs/devfs.h>

typedef struct device {
    char*         name;
    file_ops_t*         fops;
    devfs_device_type_t type;
} device_t;

static device_t devices[MAX_DEVICES] = { 0 };

file_ops_t* devfs_get_device_ops(uint32_t major) {
	if (major >= MAX_DEVICES)
		return NULL;

	return devices[major].fops;
}

int32_t devfs_register_device(uint32_t major, char* name, devfs_device_type_t type, file_ops_t* ops) {
    if (major >= MAX_DEVICES || !name || !ops)
        return -EINVAL;

    if (devices[major].fops)
        return -EBUSY;

    devices[major].name = name;
    devices[major].fops = ops;
    devices[major].type = type;

    return 0;
}

int32_t devfs_unregister_device(uint32_t major, char* name) {
    if (major >= MAX_DEVICES)
        return -EINVAL;

    if (!devices[major].fops)
        return -EINVAL;

    if (strcmp(devices[major].name, name) != 0)
        return -EINVAL;

    devices[major].name = NULL;
    devices[major].fops = NULL;
    devices[major].type = DEVFS_DEVICE_NONE;

    return 0;
}

int32_t devfs_device_open(inode_t* inode, file_t* file) {
	int32_t i = MAJOR(inode->i_rdev);

	if (i >= MAX_DEVICES)
		return -ENODEV;

	file->f_op = devices[i].fops;

	if (file->f_op->open)
		return file->f_op->open(inode, file);

	return 0;
}

file_ops_t devfs_device_fops = {
    .lseek      = NULL,
    .read       = NULL,
    .write      = NULL,
    .readdir    = NULL,
    .open       = devfs_device_open,
    .release    = NULL,
};

inode_ops_t devfs_device_iops = {
    .default_file_ops = &devfs_device_fops,

    .create     = NULL,
    .lookup     = NULL,
    .mkdir      = NULL,
    .rmdir      = NULL,
    .mknod      = NULL,
    .rename     = NULL,
};
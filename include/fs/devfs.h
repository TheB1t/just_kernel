#pragma once

#include <klibc/stdlib.h>
#include <fs/fs.h>

typedef enum {
    DEVFS_DEVICE_NONE,
    DEVFS_DEVICE_CHAR,
    DEVFS_DEVICE_BLOCK
} devfs_device_type_t;

extern inode_ops_t devfs_device_iops;

extern file_ops_t*     devfs_get_device_ops(uint32_t major);
extern int32_t         devfs_register_device(uint32_t major, char* name, devfs_device_type_t type, file_ops_t* ops);
extern int32_t         devfs_unregister_device(uint32_t major, char* name);
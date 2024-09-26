#pragma once

#include <klibc/stdlib.h>
#include <klibc/llist.h>
#include <fs/fs.h>

typedef struct vfsmount vfsmount_t;

struct vfsmount {
   dev_t            mnt_dev;
   char*            mnt_devname;
   char*            mnt_dirname;
   uint32_t         mnt_flags;
   super_block_t*   mnt_sb;
   vfsmount_t*      mnt_next;
};

extern filesystem_t*       filesystems;
extern super_block_t       super_blocks[];

extern void                iput(inode_t* inode);
extern inode_t*            iget(super_block_t* sb, int32_t ino);
extern int32_t             namei(char* pathname, inode_t** res_inode);

extern int32_t             open_namei(char* pathname, int32_t flag, int32_t mode, inode_t** res_inode, inode_t* base);

extern vfsmount_t*         lookup_vfsmnt(dev_t dev);
extern vfsmount_t*         add_vfsmnt(dev_t dev, char* dev_name, char* dir_name);
extern void                remove_vfsmnt(dev_t dev);

extern filesystem_t*       vfs_find_filesystem(char* fs_type);
extern int32_t             vfs_register_filesystem(filesystem_t* filesystem);

extern int32_t             vfs_mount(char* dev_name, char* dir_name, char* type, uint32_t new_flags, void* data);

extern int32_t             vfs_chroot(char* filename);
extern int32_t             vfs_chdir(char* filename);

extern int32_t             vfs_open(char* filename, int32_t flags, int32_t mode);
extern int32_t             vfs_creat(char* filename, int32_t mode);
extern int32_t             vfs_close(uint32_t fd);
extern int32_t             vfs_read(uint32_t fd, char* buf, uint32_t count);
extern int32_t             vfs_write(uint32_t fd, char* buf, uint32_t count);
extern int32_t             vfs_lseek(uint32_t fd, uint32_t offset, uint32_t whence);

extern int32_t             vfs_mkdir(char* pathname, int32_t mode);
extern int32_t             vfs_rmdir(char* pathname);
extern int32_t             vfs_readdir(uint32_t fd, directory_t* dirent, uint32_t count);
extern int32_t             vfs_mknod(char* filename, int32_t mode, dev_t dev);
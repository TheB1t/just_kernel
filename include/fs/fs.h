#pragma once

#include <klibc/stdlib.h>
#include <klibc/llist.h>
#include <fs/major.h>
#include <fs/stat.h>
#include <fs/fnctl.h>

#define NR_OPEN                 256
#define NR_SUPER                16

#define MINORBITS	            8
#define MINORMASK	            (BIT(MINORBITS) - 1)

#define MAJOR(dev)              ((dev) >> MINORBITS)
#define MINOR(dev)              ((dev) & MINORMASK)
#define MKDEV(mj, mi)           (((mj) << MINORBITS) | (mi))

#define MS_RDONLY	            1	/* Mount read-only */
#define MS_NODEV	            2	/* Disallow access to device special files */
#define MS_NOEXEC	            4	/* Disallow program execution */

#define IS_RDONLY(inode)        (((inode)->i_sb) && ((inode)->i_sb->s_flags & MS_RDONLY))
#define IS_NODEV(inode)         ((inode)->i_flags & MS_NODEV)
#define IS_NOEXEC(inode)        ((inode)->i_flags & MS_NOEXEC)

#define CHECK_xOP(val, p)       (val)->p##_op && (val)->p##_op
#define CHECK_IOP(inode, op)    (CHECK_xOP(inode, i)->op)
#define CHECK_SOP(sb, op)       (CHECK_xOP(sb, s)->op)
#define CHECK_FOP(file, op)     (CHECK_xOP(file, f)->op)

typedef struct super_block_ops  super_block_ops_t;
typedef struct file_ops         file_ops_t;
typedef struct inode_ops        inode_ops_t;

typedef struct directory        directory_t;
typedef struct file             file_t;
typedef struct super_block      super_block_t;
typedef struct inode            inode_t;
typedef struct filesystem       filesystem_t;

typedef uint32_t                dev_t;

struct directory {
	uint32_t		d_ino;
	uint32_t		d_off;
	uint16_t	    d_reclen;
	uint8_t		    d_name[64 + 1];
};

struct file {
    dev_t                   f_rdev;
	uint16_t                f_mode;
	int32_t			        f_pos;
	uint16_t		        f_flags;
	inode_t*                f_inode;
	file_ops_t*             f_op;
};

struct super_block {
	uint32_t		    s_blksize;

	uint32_t			s_maxbytes;
	filesystem_t*       s_type;
	uint16_t		    s_flags;
	uint32_t		    s_magic;
	inode_t*            s_covered;
	inode_t*            s_mounted;
    super_block_ops_t*  s_op;

	dev_t	            s_dev;
    dev_t		        s_rdev;
	file_t*             s_dev_file;

	void*               s_fs_info;
	char			    s_id[32];
};

struct inode {
	uint32_t	            i_dev;
    dev_t		            i_rdev;
    uint32_t                i_mode;
    uint32_t                i_ino;
    uint32_t                i_size;
    uint32_t                i_blocks;
    inode_ops_t*            i_op;
    super_block_t*          i_sb;
    inode_t*                i_next;
    inode_t*                i_prev;
    inode_t*                i_mount;
    uint16_t                i_count;
    uint16_t                i_flags;
    void*                   i_data;
};

struct super_block_ops {
    void (*read_inode)  (inode_t*);
    void (*write_inode) (inode_t*);
    void (*put_inode)   (inode_t*);
    void (*put_super)   (super_block_t*);
    void (*write_super) (super_block_t*);
};

struct file_ops {
    int32_t (*lseek)    (inode_t*, file_t*, uint32_t, uint32_t);
    int32_t (*read)     (inode_t*, file_t*, char*, uint32_t);
    int32_t (*write)    (inode_t*, file_t*, char*, uint32_t);
    int32_t (*readdir)  (inode_t*, file_t*, directory_t*, uint32_t);
    int32_t (*open)     (inode_t*, file_t*);
    void (*release)     (inode_t*, file_t*);
};

struct inode_ops {
    file_ops_t*     default_file_ops;

    int32_t (*create)   (inode_t*, char*, uint32_t, int32_t, inode_t**);
    int32_t (*lookup)   (inode_t*, char*, uint32_t, inode_t**);
    int32_t (*mkdir)    (inode_t*, char*, uint32_t, int32_t);
    int32_t (*rmdir)    (inode_t*, char*, uint32_t);
    int32_t (*mknod)    (inode_t*, char*, uint32_t, int32_t, int32_t);
    int32_t (*rename)   (inode_t*, char*, uint32_t, inode_t*, char*, int32_t);
};

struct filesystem {
    super_block_t* (*read_super)(super_block_t*, void*, int32_t);
    char* name;
    int32_t requires_dev;
    filesystem_t* next;
};
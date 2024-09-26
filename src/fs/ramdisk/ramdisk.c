#include <fs/ramdisk/ramdisk.h>
#include <fs/devfs.h>

#define RAMDISK_TYPE_START          1
#define RAMDISK_TYPE_END            2
#define RAMDISK_TYPE_FILE           3
#define RAMDISK_TYPE_DIR_START      4
#define RAMDISK_TYPE_DIR_END        5
#define RAMDISK_TYPE_DEV            6

typedef struct {
    uint8_t type;
    uint32_t size;
    char name[32];
    char data[0];
} ramdisk_entry_t;

typedef struct {
    uint8_t type;
    uint16_t rdev;
} ramdisk_dev_t;

typedef struct {
    uint32_t offset;
} ramdisk_inode_data_t;


static inline ramdisk_entry_t* _ramdisk_shift(ramdisk_entry_t* entry, uint32_t offset) {
    return (ramdisk_entry_t*)(((uint8_t*)entry) + offset);
}

static inline ramdisk_entry_t* _ramdisk_get_base(inode_t* inode) {
    return (ramdisk_entry_t*)inode->i_sb->s_fs_info;
}

static inline ramdisk_inode_data_t* _ramdisk_get_inode_data(inode_t* inode) {
    return (ramdisk_inode_data_t*)inode->i_data;
}

static inline ramdisk_entry_t* _ramdisk_get(inode_t* inode, uint32_t offset) {
    return _ramdisk_shift(_ramdisk_get_base(inode), _ramdisk_get_inode_data(inode)->offset + offset);
}

int32_t ramdisk_read(inode_t* inode, file_t* file, char* buffer, uint32_t size) {
    ramdisk_entry_t* entry = _ramdisk_get(inode, 0);
    uint8_t* data = (uint8_t*)entry->data;

    if (entry->type != RAMDISK_TYPE_FILE)
        return -ENOENT;

    uint32_t to_read = file->f_pos + size > entry->size ? entry->size - file->f_pos : size;

    memcpy(data + file->f_pos, (uint8_t*)buffer, to_read);

    file->f_pos += to_read;

    return to_read;
}

file_ops_t ramdisk_fops = {
    .lseek      = NULL,
    .read       = ramdisk_read,
    .write      = NULL,
    .readdir    = NULL,
    .open       = NULL,
    .release    = NULL
};

inode_ops_t ramdisk_file_iops = {
    .default_file_ops = &ramdisk_fops,

    .create     = NULL,
    .lookup     = NULL,
    .mkdir      = NULL,
    .rmdir      = NULL,
    .mknod      = NULL,
    .rename     = NULL
};

int32_t ramdisk_lookup(inode_t* dir, char* basename, uint32_t len, inode_t** res_inode) {
    char name[64] = { 0 };

    if (!S_ISDIR(dir->i_mode))
        return -ENOTDIR;

    memcpy((uint8_t*)basename, (uint8_t*)name, len);

    uint32_t offset = 0;

    ramdisk_entry_t* entry = _ramdisk_get(dir, offset);
    ramdisk_entry_t* base_dir = entry;
    ramdisk_entry_t* cur_dir = entry;

    // ser_printf("basedir: %s name: %s\n", base_dir->name, name);

    uint32_t current_ino = 0;
    while (entry->type != RAMDISK_TYPE_END) {
        entry = _ramdisk_get(dir, offset);
        // ser_printf("entry: %s\n", entry->name);

        if (cur_dir == base_dir && strcmp(entry->name, name) == 0) {
            *res_inode = iget(dir->i_sb, dir->i_ino + current_ino);
            return 0;
        }

        switch (entry->type) {
            case RAMDISK_TYPE_DEV:
            case RAMDISK_TYPE_FILE:
                current_ino++;
                offset += sizeof(ramdisk_entry_t) + entry->size;
                break;

            case RAMDISK_TYPE_DIR_START:
                current_ino++;
                cur_dir = entry;
                offset += sizeof(ramdisk_entry_t);
                break;

            case RAMDISK_TYPE_DIR_END:
                cur_dir = base_dir;
                offset += sizeof(ramdisk_entry_t);
                break;

            default:
                return -ENOENT;
        }
    }

    return -ENOENT;
}

inode_ops_t ramdisk_dir_iops = {
    .default_file_ops = &ramdisk_fops,

    .create     = NULL,
    .lookup     = ramdisk_lookup,
    .mkdir      = NULL,
    .rmdir      = NULL,
    .mknod      = NULL,
    .rename     = NULL
};

void ramdisk_read_inode(inode_t* inode) {
    uint32_t offset = 0;
    ramdisk_entry_t* base = _ramdisk_get_base(inode);
    ramdisk_entry_t* entry = _ramdisk_shift(base, offset);

    uint32_t current_ino = 0;
    while (entry->type != RAMDISK_TYPE_END) {
        entry = _ramdisk_shift(base, offset);

        if (current_ino == inode->i_ino && entry->type != RAMDISK_TYPE_DIR_END) {
            ramdisk_inode_data_t* data = (ramdisk_inode_data_t*)kmalloc(sizeof(ramdisk_inode_data_t));

            data->offset = offset;

            inode->i_size   = entry->size;
            inode->i_data   = (void*)data;

            switch (entry->type) {
                case RAMDISK_TYPE_DIR_START:
                    inode->i_mode = S_IFDIR;
                    inode->i_op = &ramdisk_dir_iops;
                    break;

                case RAMDISK_TYPE_DEV:
                    ramdisk_dev_t* dev = (ramdisk_dev_t*)entry->data;

                    inode->i_mode = dev->type ? S_IFBLK : S_IFCHR;
                    inode->i_rdev = dev->rdev;
                    inode->i_op = &devfs_device_iops;
                    break;

                case RAMDISK_TYPE_FILE:
                default:
                    inode->i_op = &ramdisk_file_iops;
                    inode->i_mode = S_IFREG;
                    break;
            }

            return;
        }

        switch (entry->type) {
            case RAMDISK_TYPE_FILE:
            case RAMDISK_TYPE_DEV:
                current_ino++;
                offset += sizeof(ramdisk_entry_t) + entry->size;
                break;

            case RAMDISK_TYPE_DIR_START:
                current_ino++;
                offset += sizeof(ramdisk_entry_t);
                break;

            case RAMDISK_TYPE_DIR_END:
                offset += sizeof(ramdisk_entry_t);
                break;
        }
    }
}

void ramdisk_put_inode(inode_t* inode) {
    if (inode->i_data)
        kfree(inode->i_data);

    return;
}

void ramdisk_put_super(super_block_t* sb) {
    if (sb->s_fs_info)
        kfree(sb->s_fs_info);

    return;
}

super_block_ops_t ramdisk_sbops = {
    .read_inode     = ramdisk_read_inode,
    .write_inode    = NULL,
    .put_inode      = ramdisk_put_inode,
    .put_super      = ramdisk_put_super,
    .write_super    = NULL,
};

super_block_t* ramdisk_read_super(super_block_t* sb, void* data, int32_t silent UNUSED) {
    if (!data)
        return NULL;

    uint32_t* signature = (uint32_t*)data;
    if (*signature != RAMDISK_MAGIC)
        return NULL;

    ramdisk_entry_t* ramdisk_header = (ramdisk_entry_t*)(data + sizeof(uint32_t));
    if (ramdisk_header->type != RAMDISK_TYPE_START)
        return NULL;

    ramdisk_header = (ramdisk_entry_t*)&ramdisk_header->data;
    if (ramdisk_header->type != RAMDISK_TYPE_DIR_START)
        return NULL;

    sb->s_blksize   = 0;
    sb->s_fs_info   = (void*)ramdisk_header;
    sb->s_magic     = RAMDISK_MAGIC;
    sb->s_op        = &ramdisk_sbops;
    sb->s_mounted   = iget(sb, 0);

    return sb;
}

static filesystem_t ramdisk_fs = {
    .name = "ramdisk",
    .next = NULL,
    .requires_dev = true,
    .read_super = ramdisk_read_super
};

void ramdisk_init() {
    vfs_register_filesystem(&ramdisk_fs);
}
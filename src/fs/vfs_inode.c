#include <fs/vfs.h>

static inode_t* inodes = NULL;

inode_t* lookup_inode(dev_t dev, uint32_t ino) {
    inode_t*    inode_tmp   = NULL;
    if (!inodes)
        return NULL;

    for (inode_tmp = inodes; inode_tmp; inode_tmp = inode_tmp->i_next) {
        if (inode_tmp->i_dev == dev && inode_tmp->i_ino == ino)
            return inode_tmp;
    }

    return NULL;
}

void insert_node(inode_t* inode) {
    if (!inodes) {
        inodes = inode;
        return;
    }

	inode->i_prev = NULL;
	inode->i_next = inodes;
	inodes->i_prev = inode;
	inodes = inode;
}

void remove_inode(inode_t* inode) {
   	if (!inodes)
		return;

	if (inodes == inode) {
        inodes = inode->i_next;
        return;
    }

	if (inode->i_next)
		inode->i_next->i_prev = inode->i_prev;

	if (inode->i_prev)
		inode->i_prev->i_next = inode->i_next;

	inode->i_next = NULL;
	inode->i_prev = NULL;
}

void read_inode(inode_t* inode) {
	if (!inode)
		return;

	if (inode->i_sb && CHECK_SOP(inode->i_sb, read_inode))
		inode->i_sb->s_op->read_inode(inode);
}

void iput(inode_t* inode) {
	if (!inode)
		return;

	if (!inode->i_count)
		return;

	if (inode->i_count > 1) {
		inode->i_count--;
		return;
	}

	if (inode->i_sb && CHECK_SOP(inode->i_sb, put_inode))
		inode->i_sb->s_op->put_inode(inode);

	inode->i_count--;
	if (!inode->i_count) {

		kfree(inode);
	}
}

inode_t* __iget(super_block_t* sb, uint32_t ino, int32_t crossmntp) {
	inode_t* inode  = inodes;
    inode_t* empty  = NULL;

	if (!sb)
		panic("VFS: iget with sb == NULL");

repeat:
	while (inode) {
		if (inode->i_dev != sb->s_dev || inode->i_ino != ino) {
			inode = inode->i_next;
			continue;
		}

		if (inode->i_dev != sb->s_dev || inode->i_ino != ino)
			goto repeat;

		inode->i_count++;
		if (crossmntp && inode->i_mount) {
			int32_t i;

			for (i = 0; i < NR_SUPER; i++)
				if (super_blocks[i].s_covered == inode)
					break;

			if (i >= NR_SUPER)
				return inode;

			iput(inode);
			if (!(inode = super_blocks[i].s_mounted)) {
                ser_printf("VFS: iget: sb->s_mounted is NULL\n");
			} else {
				inode->i_count++;
			}
		}

		return inode;
	}

    empty = (inode_t*)kmalloc(sizeof(inode_t));
	if (!empty)
		return NULL;

	memset((uint8_t*)empty, 0, sizeof(inode_t));

	inode = empty;
	inode->i_sb = sb;
	inode->i_dev = sb->s_dev;
	inode->i_ino = ino;
	inode->i_flags = sb->s_flags;

	insert_node(inode);
	read_inode(inode);
	return inode;
}

inode_t* iget(super_block_t* sb, int32_t ino) {
	return __iget(sb, ino, 0);
}
#include <fs/vfs.h>

vfsmount_t*      vfsmnt_list = NULL;
vfsmount_t*      vfsmnt_tail = NULL;
filesystem_t*    filesystems = NULL;

filesystem_t* vfs_find_filesystem(char* fs_type) {
    filesystem_t* current = filesystems;
    while (current) {
        if (strcmp(current->name, fs_type) == 0)
            return current;

        current = current->next;
    }

    return NULL;
}

int32_t vfs_register_filesystem(filesystem_t* filesystem) {
    if (!filesystem)
        return -EINVAL;

    filesystem_t* fs = vfs_find_filesystem(filesystem->name);
    if (fs)
        return -ENOENT;

    if (!filesystems) {
        filesystems = filesystem;
    } else {
        filesystem_t* current = filesystems;
        while (current->next)
            current = current->next;

        current->next = filesystem;
    }

    return 0;
}

vfsmount_t* lookup_vfsmnt(dev_t dev) {
	if (!vfsmnt_list)
		return NULL;

	for (vfsmount_t* lptr = vfsmnt_list; !lptr; lptr = lptr->mnt_next) {
		if (lptr->mnt_dev == dev)
			return lptr;
    }

	return NULL;
}

vfsmount_t* add_vfsmnt(dev_t dev, char* dev_name, char* dir_name) {
	vfsmount_t* lptr = (vfsmount_t*)kmalloc(sizeof(struct vfsmount));
    if (!lptr)
		return NULL;

	memset((uint8_t*)lptr, 0, sizeof(vfsmount_t));

	lptr->mnt_dev = dev;

	if (!dev_name) {
        lptr->mnt_devname = (char*)kmalloc(strlen(dev_name) + 1);

		if (lptr->mnt_devname)
			strcpy(dev_name, lptr->mnt_devname);
	}

	if (!dir_name) {
        lptr->mnt_dirname = (char*)kmalloc(strlen(dir_name) + 1);

		if (lptr->mnt_dirname)
			strcpy(dir_name, lptr->mnt_dirname);
	}

	if (!vfsmnt_list) {
		vfsmnt_list = vfsmnt_tail = lptr;
	} else {
		vfsmnt_tail->mnt_next = lptr;
		vfsmnt_tail = lptr;
	}

	return lptr;
}

void remove_vfsmnt(dev_t dev) {
    vfsmount_t* tofree = NULL;

	if (!vfsmnt_list)
		return;

	vfsmount_t* lptr = vfsmnt_list;
	if (lptr->mnt_dev == dev) {
		tofree = lptr;
		vfsmnt_list = lptr->mnt_next;
		if (vfsmnt_tail->mnt_dev == dev)
			vfsmnt_tail = vfsmnt_list;
	} else {
		while (lptr->mnt_next) {
			if (lptr->mnt_next->mnt_dev == dev)
				break;

			lptr = lptr->mnt_next;
		}

		tofree = lptr->mnt_next;
		if (!tofree)
			return;

		lptr->mnt_next = lptr->mnt_next->mnt_next;

		if (vfsmnt_tail->mnt_dev == dev)
			vfsmnt_tail = lptr;
	}

	kfree(tofree->mnt_devname);
	kfree(tofree->mnt_dirname);
	kfree(tofree);
}
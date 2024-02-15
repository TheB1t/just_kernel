#!/bin/bash

IMAGE_FILE=disk
IMAGE_SIZE_MB=32
LOOP_DEV=loop100
MOUNT_DIR=image
PXE_DIR=/var/lib/tftpboot
GRUB_CFG_PATH=grub.cfg


#  Remove old image
function remove_image() {
    rm ${IMAGE_FILE}.img
}

#  Create file
function create_image() {
    dd if=/dev/zero of=${IMAGE_FILE}.img bs=1M count=${IMAGE_SIZE_MB}
}

#  Create loop device
function create_loop() {
    sudo losetup -P /dev/${LOOP_DEV} ${IMAGE_FILE}.img
}

#  Create ext2 filesystem
function make_filesystem() {
    sudo parted -s /dev/${LOOP_DEV} mklabel msdos
    sudo parted -s /dev/${LOOP_DEV} mkpart primary ext2 1MiB 100%

    sudo mkfs.ext2 /dev/${LOOP_DEV}p1
}

#  Mount loop device
function mount_loop() {
    mkdir ${MOUNT_DIR}

    sudo mount /dev/${LOOP_DEV}p1 $MOUNT_DIR
}

#  Install grub
function install_grub() {
    sudo grub-install --root-directory=${MOUNT_DIR} --boot-directory=${MOUNT_DIR}/boot --no-floppy --modules="normal part_msdos ext2 multiboot" /dev/${LOOP_DEV}
}

#  Umount loop device
function umount_loop() {
    sudo umount /dev/${LOOP_DEV}p1

    rmdir ${MOUNT_DIR}
}

#  Remove loop device
function remove_loop() {
    sudo losetup -D /dev/${LOOP_DEV}
}

function main() {
    local action=$1; shift

	case "$action" in
    "init")
        remove_image
        create_image
        create_loop
        make_filesystem
        mount_loop
        install_grub

        umount_loop
        remove_loop
        ;;
	"pack")
        create_loop
        mount_loop

        sudo rm ${MOUNT_DIR}/boot/grub/grub.cfg
        sudo cp ${GRUB_CFG_PATH} ${MOUNT_DIR}/boot/grub/
        
        cmake -B build .
        make -C build

        sudo rm ${MOUNT_DIR}/kernel

        sudo cp build/bin/kernel ${MOUNT_DIR}/

        umount_loop
        remove_loop
	    ;;
    "pack_pxe")
        sudo rm ${PXE_DIR}/boot/grub/grub.cfg
        sudo cp ${GRUB_CFG_PATH} ${PXE_DIR}/boot/grub/

        cmake -B build .
        make -C build

        sudo rm ${PXE_DIR}/kernel

        sudo cp build/bin/kernel ${PXE_DIR}/ 
        ;;
    "run")
        sudo qemu-system-x86_64 -hda ${IMAGE_FILE}.img -cpu host --enable-kvm -smp 8,sockets=2,cores=2,threads=2 -m 1024M -machine q35 -no-reboot -serial stdio
        ;;
    "run_debug")
        sudo qemu-system-x86_64 -s -S -hda ${IMAGE_FILE}.img -machine q35 -no-reboot -serial stdio
        ;;
	"mount")
        create_loop
        mount_loop
	    ;;
    "umount")
        umount_loop
        remove_loop
        ;;
	*)
	    [[ -n "$action" ]] && echo "Not implemented action $1" || echo "Action not passed"
	    echo "Avaliable actions:"
	    echo "	pack                : packing kernel"
	    echo "	mount               : mount image"
	    echo "	umount              : umount image"	    
        ;;
	esac

}

main $@
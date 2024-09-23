#!/bin/bash

IMAGE_FILE=disk
IMAGE_SIZE_MB=32
LOOP_DEV=loop100
MOUNT_DIR=image
PXE_DIR=/var/lib/tftpboot
GRUB_CFG_PATH=grub.cfg

function get_color() {
    case $1 in
        red)        echo "\033[31m" ;;
        green)      echo "\033[32m" ;;
        yellow)     echo "\033[33m" ;;
        blue)       echo "\033[34m" ;;
        magenta)    echo "\033[35m" ;;
        cyan)       echo "\033[36m" ;;
    esac
}

function reset_color() {
    echo -ne "\033[0m"
}

function _log() {
    local type=$3
    local msg=$4

    local color_code0=$(get_color $1)
    local color_code1=$(get_color $2)

    echo -e "${color_code0}[${type}]${color_code1} ${msg}"
    reset_color
}

function log() {
    _log green yellow "${@}"
}

function log_err() {
    _log red yellow ERR "${@}"
}

function run() {
    log RUN "${*}"

    local color_code0=$(get_color red)
    "${@}"
}

#  Remove old image
function remove_image() {
    run rm ${IMAGE_FILE}.img
}

#  Create file
function create_image() {
    run dd if=/dev/zero of=${IMAGE_FILE}.img bs=1M count=${IMAGE_SIZE_MB}
}

#  Create loop device
function create_loop() {
    run sudo losetup -P /dev/${LOOP_DEV} ${IMAGE_FILE}.img
}

#  Create ext2 filesystem
function make_filesystem() {
    run sudo parted -s /dev/${LOOP_DEV} mklabel msdos
    run sudo parted -s /dev/${LOOP_DEV} mkpart primary ext2 1MiB 100%

    run sudo mkfs.ext2 /dev/${LOOP_DEV}p1
}

#  Mount loop device
function mount_loop() {
    run mkdir ${MOUNT_DIR}

    run sudo mount /dev/${LOOP_DEV}p1 $MOUNT_DIR
}

#  Install grub
function install_grub() {
    run sudo grub-install --target=i386-pc --root-directory=${MOUNT_DIR} --boot-directory=${MOUNT_DIR}/boot --no-floppy --modules="normal part_msdos ext2 multiboot" /dev/${LOOP_DEV}
}

#  Umount loop device
function umount_loop() {
    run sudo umount /dev/${LOOP_DEV}p1

    run rmdir ${MOUNT_DIR}
}

#  Remove loop device
function remove_loop() {
    run sudo losetup -D /dev/${LOOP_DEV}
}

function build() {
    local type=$1

    if [ -z "${type}" ]; then
        type=Debug
    fi

    log BUILD "Type: ${type}"
    run cmake -B build . -DCMAKE_BUILD_TYPE=${type}
    run make -C build
}

function pack() {
    local dst=$1

    run sudo rm ${dst}/boot/grub/grub.cfg
    run sudo rm ${dst}/kernel
    run sudo cp ${GRUB_CFG_PATH} ${dst}/boot/grub/
    run sudo cp build/bin/kernel ${dst}/
}

#  Run qemu
function run_qemu() {
    run sudo qemu-system-x86_64 -m 1024M -hda ${IMAGE_FILE}.img -no-reboot -no-shutdown -serial stdio ${@}
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

        build $@
        pack ${MOUNT_DIR}

        umount_loop
        remove_loop
	    ;;
    "pack_pxe")
        build $@
        pack ${PXE_DIR}
        ;;
    "runb")
        run rm serial.txt
        run rm ${IMAGE_FILE}.img.lock
        run bochs
        ;;
    "run")
        run_qemu -machine q35 --cpu max -smp 4
        ;;
    "runk")
        run_qemu -machine q35 --enable-kvm --cpu host -smp 4
        ;;
    "rund")
        run_qemu -s -S
        ;;
	"mount")
        create_loop
        mount_loop
	    ;;
    "umount")
        umount_loop
        remove_loop
        ;;
    "a2l")
        local addr=$1

        if [ -z "${addr}" ]; then
            log_err "Address not passed"
            exit 1
        fi

        if [ ! -f build/bin/kernel ]; then
            log_err "Kernel not found (build it first)"
            exit 1
        fi

        run addr2line -e build/bin/kernel ${addr}
        ;;
    "burn")
        local drive=$1

        if [ -z "${drive}" ]; then
            log_err "Drive not passed"
            exit 1
        fi

        if [ ! -f ${IMAGE_FILE}.img ]; then
            log_err "Image not found (pack it first)"
            exit 1
        fi

        run sudo dd if=${IMAGE_FILE}.img of=${drive} status=progress
        ;;
	*)
	    [[ -n "$action" ]] && log_err "Not implemented action $1" || log_err "Action not passed"
	    echo "Avaliable actions:"
        echo "	run                     : run qemu"
        echo "	runk                    : run qemu with kvm"
        echo "	rund                    : run qemu with debug (gdb server)"
        echo "	runb                    : run bochs"
        echo "	burn <drive>            : burn image to drive"
        echo "	init                    : init image"
        echo "	mount                   : mount image"
        echo "	umount                  : umount image"
        echo "	a2l <addr>              : addr2line"
        echo "	burn <drive>            : burn image to drive"
        echo "	pack <build_type>       : packing kernel"
        echo "	pack_pxe <build_type>   : packing kernel for pxe"
        ;;
	esac

}

main $@
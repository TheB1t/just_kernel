# µOS kernel

__Version:__ 0.1

### About

A hybrid kernel for µOS

### What tools i using for development

Compilers and languages:
- `GCC` as `C` code compiler
- `YASM` as `ASM` code compiler
- `LD` as linker

For writing code:
- Visual Studio Code
- [micro](https://github.com/zyedidia/micro?ysclid=l9jqlbouhf912724444)

For debugging kernel:
- qemu
- bochs
- gdb
- real hardware

For building:
- make
- cmake

#### Hardware list

| Manufacturer | Model | Type        | Chipset   | Processor               | RAM                                 |
| ------------ | ----- | ----------- | --------- | ----------------------- | ----------------------------------- |
| HP           | T5570 | Thin Client | VIA VX900 | VIA Nano U3500, 1000MHz | `1Gb Samsung M471B2873FHS-CH9` x1   |


### What has been implemented now and what is planned

- [x] Boot
    - [x] Loading from multiboot-compliant bootloader
- [x] Memory & Interrupts
    - [x] Support of `GDT`, `LDT` and `IDT` cofiguration
    - [x] Memory manager
        - [x] Physical memory manager
        - [x] Virtual memory manager
        - [x] kmalloc & kfree for kernel purposes
- [ ] Tasking
- [x] Drivers
    - [x] early_screen driver
    - [x] serial driver
    - [ ] VESA driver
    - [ ] PCI driver
    - [ ] Ethernet driver
- [ ] Module support
- [ ] Create cross-compiler
- [ ] Port C library ([mlibc](https://github.com/managarm/mlibc/tree/master)?)
- [ ] Port GCC

### Requirements for build and run

1. Install this packages (if you using debian-based system):
```bash
sudo apt -y install qemu qemu-system nasm yasm gcc g++ binutils make cmake grub-pc
```

### How to build/pack/run it

1. Create image
```bash
./utils.sh init
```

2. Build and pack kernel into image:
```bash
./utils.sh pack
```

2. Run using `qemu`:
```bash
./utils.sh run
```

or you can run the build manually using:
```bash
cmake -B build .
make -C build
```
the compiled binaries will be located in the `build/bin` folder.

### How to contact me

__Email:__ bithovalsky@gmail.com
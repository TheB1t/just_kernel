# pixOS

### What is this?

This is a hobby project of mine, where i'm trying to create my own operating system (4th try).

But honestly, only god (and i) knows what kind of thing this is ✝️️. Happy reading!

### What tools i using for development

Debugging tools & things:
- qemu
- bochs
- gdb
- real hardware

Compile & build tools:
- gcc
- yasm
- cmake

#### Hardware list

| Manufacturer | Model | Type        | Chipset   | Processor               | RAM                                 |
| ------------ | ----- | ----------- | --------- | ----------------------- | ----------------------------------- |
| HP           | T5570 | Thin Client | VIA VX900 | VIA Nano U3500, 1000MHz | `1Gb Samsung M471B2873FHS-CH9` x1   |

### What has been implemented now and what is planned

- [x] Boot
    - [x] Loading from multiboot-compliant bootloader
    - [x] Kernel symbols loading
    - [x] Parsing memory map from bootloader
- [x] Memory & Interrupts
    - [x] Support of `GDT`, `LDT` and `IDT` configuration
    - [x] Memory manager
        - [x] PMM
        - [x] VMM
        - [x] Heap
            - [x] kmalloc & kfree for kernel purposes
            - [x] Each process has it's own heap
- [x] Tasking
    - [x] Process
        - [x] Create and execute
        - [ ] Destroy
    - [x] Thread
        - [x] Create and execute
        - [ ] Destroy
    - [x] Sync primitives
        - [x] Spinlock
        - [ ] Mutex
        - [ ] Semaphore
    - [x] Syscalls
    - [x] Multicore
        - [x] SMP init
        - [x] TSS init for each core
        - [x] Core locals
- [x] Other
    - [ ] APIC
        - [x] MADT parsing
    - [x] Stack trace
- [x] Executables
    - [x] Formats
        - [x] ELF
    - [ ] Dynamicly linked
- [x] Drivers
    - [x] early_screen driver
    - [x] serial driver
    - [ ] VESA driver
    - [ ] PCI driver
    - [ ] ATA driver
    - [ ] Ethernet driver
- [ ] VFS
    - [ ] Filesystem
        - [ ] ext2
        - [ ] FAT32
- [ ] Module support
- [ ] Build cross-compiler
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

3. Running
    1. Using `qemu`:

    ```bash
    ./utils.sh run
    ```

    2. Using `qemu` with kvm:

    ```bash
    ./utils.sh runk
    ```

    3. Using `bochs`:

    ```bash
    ./utils.sh runb
    ```

or you can run the build manually using:

```bash
cmake -B build .
make -C build
```

and run as you prefer. Compiled binaries will be located in the `build/bin` folder.

### Contributing

If you want to contribute to this project, you can do so by following these steps:

1. Fork this repo
2. Create a branch
3. Commit your changes
4. Create a pull request
5. Wait for review

### How to contact me

__Email:__ bithovalsky@gmail.com

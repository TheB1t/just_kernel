#pragma once

#include <klibc/stdlib.h>
#include <fs/vfs.h>

#define RAMDISK_MAGIC               0x495f5244

void ramdisk_init();
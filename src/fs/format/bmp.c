#include <fs/format/bmp.h>
#include <fs/vfs.h>
#include <drivers/vesa.h>

void bmp_draw(uint32_t x, uint32_t y, int32_t fd) {
    bmp_header_t header = { 0 };
    bmp_info_header_t info = { 0 };
    int32_t readed = 0;

    if (fd < 0)
        return;

    readed = vfs_read(fd, (char*)&header, sizeof(bmp_header_t));
    if (readed != sizeof(bmp_header_t)) {
        ser_printf("[BMP] Error reading header\n");
        return;
    }

    if (header.type != BMP_MAGIC) {
        ser_printf("[BMP] Invalid signature\n");
        return;
    }

    readed = vfs_read(fd, (char*)&info, sizeof(bmp_info_header_t));
    if (readed != sizeof(bmp_info_header_t)) {
        ser_printf("[BMP] Error reading info header\n");
        return;
    }

    if (info.size != sizeof(bmp_info_header_t)) {
        ser_printf("[BMP] Invalid info header size\n");
        return;
    }

    ser_printf("[BMP] Picture size: %d x %d\n", info.width, info.height);
    ser_printf("[BMP] Planes: %d\n", info.planes);
    ser_printf("[BMP] Bits-per-pixel: %d\n", info.bit_count);
    ser_printf("[BMP] Compression: %d\n", info.compression);
    ser_printf("[BMP] Pixel data size: %d\n", info.size_image);

    if (info.bit_count != 24 && info.bit_count != 32) {
        ser_printf("[BMP] Unsupported bit count\n");
        return;
    }

    if (info.compression != 0) {
        ser_printf("[BMP] Unsupported compression\n");
        return;
    }

	uint8_t* pixeldata = (uint8_t*)kmalloc(info.size_image);
    if (!pixeldata) {
        ser_printf("[BMP] Error allocating memory for pixel data\n");
        return;
    }

    readed = vfs_read(fd, (char*)pixeldata, info.size_image);
    if ((uint32_t)readed != info.size_image) {
        ser_printf("[BMP] Error reading pixel data\n");
        goto error;
    }

    vesa_vector2_t v = {
        .x = x,
        .y = y,
    };

    vesa_draw_picture(v, info.width, info.height, info.bit_count, pixeldata);

error:
    kfree(pixeldata);
}
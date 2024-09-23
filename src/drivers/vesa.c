#include <drivers/vesa.h>
#include <mm/vmm.h>
#include <proc/v86.h>

vesa_video_mode_t   early_mode      = {0};
vesa_video_mode_t*  current_mode    = NULL;
vesa_video_mode_t*  vesa_modes      = NULL;

void vesa_init_early(multiboot_info_t* mboot) {
    current_mode = &early_mode;

    current_mode->width        = mboot->framebuffer_width;
    current_mode->height       = mboot->framebuffer_height;
    current_mode->pitch        = mboot->framebuffer_pitch;
    current_mode->bpp          = mboot->framebuffer_bpp;
    current_mode->fb_size      = current_mode->height * current_mode->pitch;
    current_mode->fb           = (uint8_t*)((uint32_t)mboot->framebuffer_addr);

    switch (mboot->framebuffer_type) {
        case MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT:
            current_mode->fb_type = VESA_MODE_TEXT;
            break;
        case MULTIBOOT_FRAMEBUFFER_TYPE_RGB:
            current_mode->fb_type = VESA_MODE_GRAPHIC;

            current_mode->r_mask = (1 << mboot->framebuffer_red_mask_size) - 1;
            current_mode->g_mask = (1 << mboot->framebuffer_green_mask_size) - 1;
            current_mode->b_mask = (1 << mboot->framebuffer_blue_mask_size) - 1;
            current_mode->a_mask = 0;
            current_mode->r_pos  = mboot->framebuffer_red_field_position;
            current_mode->g_pos  = mboot->framebuffer_green_field_position;
            current_mode->b_pos  = mboot->framebuffer_blue_field_position;
            current_mode->a_pos  = 0;
            break;
        default:
            panic("Unknown framebuffer type");
    };

    vmm_map((void*)current_mode->fb, (void*)current_mode->fb, ALIGN(current_mode->fb_size) / PAGE_SIZE, VMM_PRESENT | VMM_WRITE);

    ser_printf("[VESA] Framebuffer address: 0x%08x (size %d, pages %d)\n", current_mode->fb, current_mode->fb_size, ALIGN(current_mode->fb_size) / PAGE_SIZE);
    ser_printf("[VESA] Display info:\n");
    ser_printf("    - type: %s\n", current_mode->fb_type == VESA_MODE_TEXT ? "text" : "graphic");
    ser_printf("    - pitch: %u\n", current_mode->pitch);
    ser_printf("    - width: %u\n", current_mode->width);
    ser_printf("    - height: %u\n", current_mode->height);
    ser_printf("    - bpp: %u\n", current_mode->bpp);
}

bool _vesa_get_info() {
    v86_call_t call = {
        .ctx = { .ax = 0x4F00, .si = 0x0000, .di = 0xD000 },
        .int_num = 0x10,
        .type = V86_CALL_TYPE_GENERIC_INT,
    };

    v86_call(&call);

    return call.ctx.ax == 0x004F;
}

bool _vesa_get_mode_info(uint16_t mode) {
    v86_call_t call = {
        .ctx = { .ax = 0x4F01, .cx = mode, .si = 0x0000, .di = 0xE000 },
        .int_num = 0x10,
        .type = V86_CALL_TYPE_GENERIC_INT,
    };

    v86_call(&call);

    return call.ctx.ax == 0x004F;
}

bool _vesa_set_mode(uint16_t mode) {
    mode &= ~0x800;

    v86_call_t call = {
        .ctx = { .ax = 0x4F02, .bx = mode },
        .int_num = 0x10,
        .type = V86_CALL_TYPE_GENERIC_INT,
    };

    v86_call(&call);

    return call.ctx.ax == 0x004F;
}

void vesa_init() {
    vbe_info_t*         info        = (vbe_info_t*)0x0000D000;
    vbe_mode_info_t*    mode_info   = (vbe_mode_info_t*)0x0000E000;

    memset((uint8_t*)info, 0, sizeof(vbe_info_t));
    memcpy((uint8_t*)"VBE2", (uint8_t*)info->signature, 4);

    if (!_vesa_get_info()) {
        ser_printf("[VESA] VBE not supported\n");
        return;
    }

    uint16_t* modes = (uint16_t*)v86_farptr2linear(info->video_modes);
    for (uint32_t i = 0; modes[i] != 0xFFFF; ++i) {
        if (!_vesa_get_mode_info(modes[i]))
            continue;

        if ((mode_info->attributes & 0x90) != 0x90)
            continue;

        if (mode_info->memory_model != 4 && mode_info->memory_model != 6)
            continue;

        vesa_video_mode_t* mode = kmalloc(sizeof(vesa_video_mode_t));

        mode->mode          = modes[i];
        mode->width         = mode_info->width;
        mode->height        = mode_info->height;
        mode->pitch         = mode_info->pitch;
        mode->bpp           = mode_info->bpp;
        mode->fb_size       = mode_info->pitch * mode_info->height;
        mode->fb            = (uint8_t*)mode_info->framebuffer;

        if (mode_info->memory_model) {
            mode->fb_type = VESA_MODE_GRAPHIC;

            mode->r_mask = (1 << mode_info->red_mask) - 1;
            mode->g_mask = (1 << mode_info->green_mask) - 1;
            mode->b_mask = (1 << mode_info->blue_mask) - 1;
            mode->a_mask = (1 << mode_info->reserved_mask) - 1;
            mode->r_pos  = mode_info->red_position;
            mode->g_pos  = mode_info->green_position;
            mode->b_pos  = mode_info->blue_position;
            mode->a_pos  = mode_info->reserved_position;
        } else {
            mode->fb_type = VESA_MODE_TEXT;
        }

        if (vesa_modes)
            mode->next = vesa_modes;
        else
            mode->next = NULL;

        vesa_modes = mode;
    }
}

vesa_video_mode_t* vesa_find_mode(uint32_t width, uint32_t height, uint8_t bpp) {
    vesa_video_mode_t* mode = vesa_modes;
    while (mode) {
        if (mode->width == width && mode->height == height && mode->bpp == bpp)
            return mode;

        mode = mode->next;
    }

    return NULL;
}

void vesa_set_mode(vesa_video_mode_t* mode) {
    if (!mode)
        return;

    if (!_vesa_set_mode(mode->mode | 0x4000)) {
        ser_printf("[VESA] VBE mode 0x%04x not supported\n", mode);
        return;
    }

    current_mode = mode;

    vmm_map((void*)current_mode->fb, (void*)current_mode->fb, ALIGN(current_mode->fb_size) / PAGE_SIZE, VMM_PRESENT | VMM_WRITE);

    ser_printf("[VESA] Framebuffer address: 0x%08x (size %d, pages %d)\n", current_mode->fb, current_mode->fb_size, ALIGN(current_mode->fb_size) / PAGE_SIZE);
    ser_printf("[VESA] Display info:\n");
    ser_printf("    - type: %s\n", current_mode->fb_type == VESA_MODE_TEXT ? "text" : "graphic");
    ser_printf("    - pitch: %u\n", current_mode->pitch);
    ser_printf("    - width: %u\n", current_mode->width);
    ser_printf("    - height: %u\n", current_mode->height);
    ser_printf("    - bpp: %u\n", current_mode->bpp);
}

void vesa_switch_to_best_mode() {
    vesa_video_mode_t* best = vesa_modes;
    vesa_video_mode_t* mode = vesa_modes;
    while (mode) {
        if (mode->width > best->width ||
            (mode->width == best->width && mode->height > best->height) ||
            (mode->width == best->width && mode->height == best->height && mode->bpp > best->bpp)) {
            best = mode;
        }

        mode = mode->next;
    }

    vesa_set_mode(best);
}

void vesa_put_symbol(vesa_vector2_t pos, char c, vesa_color_t bg, vesa_color_t fg) {
    uint32_t idx = (pos.y * current_mode->pitch) + (pos.x * (current_mode->bpp / 8));
    uint8_t* fb_entry = (uint8_t*)&current_mode->fb[idx];

    fb_entry[0] = c;
    fb_entry[1] = bg.raw << 4 | fg.raw;
}

static inline void vesa_color2entry(uint8_t* entry, vesa_color_t* color) {
    if (current_mode->fb_type == VESA_MODE_TEXT) {
        entry[0] = ' ';
        entry[1] = color->raw << 4 | color->raw;
    } else {
        uint32_t r = (color->r & current_mode->r_mask) << current_mode->r_pos;
        uint32_t g = (color->g & current_mode->g_mask) << current_mode->g_pos;
        uint32_t b = (color->b & current_mode->b_mask) << current_mode->b_pos;
        uint32_t a = (color->a & current_mode->a_mask) << current_mode->a_pos;

        uint32_t full_color = (r | g | b | a);

        memcpy((uint8_t*)&full_color, entry, current_mode->bpp / 8);
    }
}

void vesa_render_symbol(vesa_vector2_t pos, char c, font_t* font, vesa_color_t bg, vesa_color_t fg) {
    for (uint8_t iy = 0; iy < font->height; iy++) {
        for (uint8_t ix = 0; ix < font->width; ix++) {
            uint8_t pixel = (font->data[((uint8_t)c * font->height) + iy] >> ix) & 1;

            uint32_t y = pos.y + iy;
            uint32_t x = pos.x + ix;
            uint32_t idx = (y * current_mode->pitch) + (x * (current_mode->bpp / 8));

            vesa_color2entry(&current_mode->fb[idx], pixel ? &fg : &bg);
        }
    }
}

void vesa_putc(vesa_vector2_t pos, char c, font_t* font, vesa_color_t bg, vesa_color_t fg) {
    switch (current_mode->fb_type) {
        case VESA_MODE_TEXT:        vesa_put_symbol(pos, c, bg, fg); break;
        case VESA_MODE_GRAPHIC:     vesa_render_symbol(pos, c, font, bg, fg); break;
        default:
            panic("Unknown framebuffer type");
    }
}

void vesa_scroll(uint32_t rows, vesa_color_t bg) {
    uint32_t rows_shift = rows * current_mode->pitch;
    uint32_t delta      = current_mode->fb_size - rows_shift;

    for (uint32_t i = delta; i < current_mode->fb_size; i += current_mode->bpp)
        vesa_color2entry(&current_mode->fb[i], &bg);

    memcpy((uint8_t*)(current_mode->fb + rows_shift), (uint8_t*)current_mode->fb, delta);
}

void vesa_draw_picture(vesa_vector2_t pos, uint32_t width, uint32_t height, uint8_t* data) {
    typedef struct {
        uint8_t     r;
        uint8_t     g;
        uint8_t     b;
    } __attribute__((packed)) picture_pixel_t;

    picture_pixel_t* pixels = (picture_pixel_t*)data;

    ser_printf("Drawing picture at %d, %d (size %d x %d)\n", pos.x, pos.y, width, height);

    for (uint32_t iy = 0; iy < height; iy++) {
        for (uint32_t ix = 0; ix < width; ix++) {
            picture_pixel_t* pixel = &pixels[(iy * width) + ix];

            uint32_t y = pos.y + iy;
            uint32_t x = pos.x + ix;
            uint32_t idx = (y * current_mode->pitch) + (x * (current_mode->bpp / 8));

            vesa_color_t color = {
                .r = pixel->r,
                .g = pixel->g,
                .b = pixel->b,
            };

            vesa_color2entry(&current_mode->fb[idx], &color);
        }
    }
}
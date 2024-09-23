#pragma once

#include <klibc/stdlib.h>
#include <multiboot.h>
#include <drivers/font.h>

typedef struct {
    uint32_t    x;
    uint32_t    y;
} vesa_vector2_t;

typedef enum {
    TEXT_COLOR_BLACK           = 0,
    TEXT_COLOR_BLUE            = 1,
    TEXT_COLOR_GREEN           = 2,
    TEXT_COLOR_CYAN            = 3,
    TEXT_COLOR_RED             = 4,
    TEXT_COLOR_PURPLE          = 5,
    TEXT_COLOR_BROWN           = 6,
    TEXT_COLOR_LIGHT_GREY      = 7,
    TEXT_COLOR_DARK_GREY       = 8,
    TEXT_COLOR_LIGHT_BLUE      = 9,
    TEXT_COLOR_LIGHT_GREEN     = 10,
    TEXT_COLOR_LIGHT_CYAN      = 11,
    TEXT_COLOR_LIGHT_RED       = 12,
    TEXT_COLOR_LIGHT_PURPLE    = 13,
    TEXT_COLOR_YELLOW          = 14,
    TEXT_COLOR_WHITE           = 15,
} vesa_text_color_t;

typedef struct {
    uint32_t    raw;

    uint32_t    b : 8;
    uint32_t    g : 8;
    uint32_t    r : 8;
    uint32_t    a : 8;
} vesa_color_t;

#define VESA_TEXT_COLOR(_color)                 ((vesa_color_t){ .raw = (_color) })
#define VESA_GRAPHICS_COLOR(_r, _g, _b, _a)     ((vesa_color_t){ .r = (_r), .g = (_g), .b = (_b), .a = (_a)})

typedef enum {
    VESA_MODE_TEXT,
    VESA_MODE_GRAPHIC
} vesa_mode_t;

typedef struct vesa_video_mode {
    uint16_t    mode;

    uint8_t*    fb;
    vesa_mode_t fb_type;
    uint32_t    fb_size;

    uint32_t    width;
    uint32_t    height;
    uint32_t    pitch;
    uint8_t     bpp;

	uint8_t     r_mask;
	uint8_t     r_pos;
	uint8_t     g_mask;
	uint8_t     g_pos;
	uint8_t     b_mask;
	uint8_t     b_pos;
	uint8_t     a_mask;
	uint8_t     a_pos;

    struct vesa_video_mode* next;
} vesa_video_mode_t;

typedef struct {
	char        signature[4];
	uint16_t    version;
	uint32_t    oem;
	uint32_t    capabilities;
	uint32_t    video_modes;
	uint16_t    video_memory;
	uint16_t    software_rev;
	uint32_t    vendor;
	uint32_t    product_name;
	uint32_t    product_rev;
	char        reserved[222];
	char        oem_data[256];
} __attribute__ ((packed)) vbe_info_t;

typedef struct {
	uint16_t    attributes;
	uint8_t     window_a;
	uint8_t     window_b;
	uint16_t    granularity;
	uint16_t    window_size;
	uint16_t    segment_a;
	uint16_t    segment_b;
	uint32_t    win_func_ptr;
	uint16_t    pitch;
	uint16_t    width;
	uint16_t    height;
	uint8_t     w_char;
	uint8_t     y_char;
	uint8_t     planes;
	uint8_t     bpp;
	uint8_t     banks;
	uint8_t     memory_model;
	uint8_t     bank_size;
	uint8_t     image_pages;
	uint8_t     reserved0;

	uint8_t     red_mask;
	uint8_t     red_position;
	uint8_t     green_mask;
	uint8_t     green_position;
	uint8_t     blue_mask;
	uint8_t     blue_position;
	uint8_t     reserved_mask;
	uint8_t     reserved_position;
	uint8_t     direct_color_attributes;

	uint32_t    framebuffer;
	uint32_t    off_screen_mem_off;
	uint16_t    off_screen_mem_size;
	uint8_t     reserved1[206];
} __attribute__ ((packed)) vbe_mode_info_t;

typedef struct {
	uint16_t set_window;
	uint16_t set_display_start;
	uint16_t set_pallette;
} __attribute__ ((packed)) vbe2_pmi_table_t;

extern vesa_video_mode_t* current_mode;

void                    vesa_init_early(multiboot_info_t* mboot);
void                    vesa_init();
vesa_video_mode_t*      vesa_find_mode(uint32_t width, uint32_t height, uint8_t bpp);
void                    vesa_set_mode(vesa_video_mode_t* mode);
void					vesa_switch_to_best_mode();
void                    vesa_putc(vesa_vector2_t pos, char c, font_t* font, vesa_color_t bg, vesa_color_t fg);
void                    vesa_scroll(uint32_t rows, vesa_color_t bg);
void                    vesa_draw_picture(vesa_vector2_t pos, uint32_t width, uint32_t height, uint8_t* data);
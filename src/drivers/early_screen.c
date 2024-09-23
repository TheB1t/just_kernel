#include <drivers/early_screen.h>

vesa_vector2_t	cursor = { .x = 0, .y = 0 };

vesa_color_t	fg_color_graphic	= VESA_GRAPHICS_COLOR(255, 255, 255, 0);
vesa_color_t	bg_color_graphic	= VESA_GRAPHICS_COLOR(0, 0, 0, 0);

vesa_color_t	fg_color_text	= VESA_TEXT_COLOR(TEXT_COLOR_WHITE);
vesa_color_t	bg_color_text	= VESA_TEXT_COLOR(TEXT_COLOR_BLACK);

static inline vesa_vector2_t get_symbol_size() {
	if (current_mode->fb_type == VESA_MODE_TEXT)
		return (vesa_vector2_t){
			.x = 1,
			.y = 1
		};

	return (vesa_vector2_t){
		.x = font8x8_basic.width + 2,
		.y = font8x8_basic.height + 2
	};
}

static inline vesa_color_t get_color(bool fg) {
	if (current_mode->fb_type == VESA_MODE_TEXT)
		return fg ? fg_color_text : bg_color_text;

	return fg ? fg_color_graphic : bg_color_graphic;
}

void screen_move_cursor() {
	if (current_mode->fb_type != VESA_MODE_TEXT)
		return;

	uint16_t cursor_location = (cursor.y * current_mode->width) + cursor.x;

	port_outb(0x3D4, 14);
	port_outb(0x3D5, cursor_location >> 8);
	port_outb(0x3D4, 15);
	port_outb(0x3D5, cursor_location);
}

void screen_scroll() {
	vesa_vector2_t size = get_symbol_size();

	if (cursor.y >= current_mode->height - size.y) {
		vesa_scroll(size.y, get_color(false));

		cursor.y -= size.y;
	}
}

void screen_putc(char c) {
	vesa_vector2_t size = get_symbol_size();

	switch (c) {
		case '\b':
			if (cursor.x) {
				cursor.x -= size.x;
				vesa_putc(cursor, ' ', &font8x8_basic, get_color(false), get_color(true));
			}
			break;

		case '\t':
			cursor.x = (cursor.x + (4 - (cursor.x % 4)));// & ~(8 - 1);
			break;

		case '\n':
			cursor.y += size.y;
			cursor.x = 0;
			break;

		case '\r':
			cursor.x = 0;
			break;

		default:
			if (c >= ' ') {
				vesa_putc(cursor, c, &font8x8_basic, get_color(false), get_color(true));
				cursor.x += size.x;
			}
	}

	if (cursor.x >= current_mode->width) {
		cursor.x = 0;
		cursor.y += size.y;
	}

	screen_scroll();
	screen_move_cursor();
}

void screen_clear() {
	cursor.x = 0;
	cursor.y = 0;
	screen_move_cursor();
}

void screen_puts(char* c) {
	while (*c) {
		screen_putc(*c++);
	}
}

void screen_init() {
    _global_putchar = screen_putc;
	screen_clear();
}
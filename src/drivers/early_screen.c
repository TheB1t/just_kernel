#include <drivers/early_screen.h>

vesa_vector2_t	cursor = { .x = 0, .y = 0 };

vesa_color_t	graphics_text_color	= VESA_GRAPHICS_RGB888(255, 255, 255);
vesa_color_t	graphics_bg_color	= VESA_GRAPHICS_RGB888(0, 0, 0);
vesa_color_t	color_text			= VESA_TEXT_ATTRIBUTE(TEXT_COLOR_BLACK, TEXT_COLOR_WHITE);

static inline vesa_vector2_t get_symbol_size() {
	if (current_mode->fb_type == VESA_MODE_TEXT)
		return (vesa_vector2_t){
			.x = 1,
			.y = 1
		};

	return (vesa_vector2_t){
		.x = font8x8_basic.width,
		.y = font8x8_basic.height + 2
	};
}

static inline void _putc(char c) {
	if (current_mode->fb_type == VESA_MODE_TEXT)
		vesa_put_symbol(cursor, c, color_text);
	else
		vesa_render_symbol(cursor, c, &font8x8_basic, graphics_text_color);
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
		if (current_mode->fb_type == VESA_MODE_TEXT)
			vesa_scroll(size.y, color_text);
		else
			vesa_scroll(size.y, graphics_bg_color);

		cursor.y -= size.y;
	}
}

void screen_putc(char c) {
	vesa_vector2_t size = get_symbol_size();

	switch (c) {
		case '\b':
			if (cursor.x) {
				cursor.x -= size.x;
				_putc(c);
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
				_putc(c);
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

void _screen_putchar(char c, void* arg UNUSED) {
	screen_putc(c);
}

void screen_init() {
    _global_putchar = _screen_putchar;
	screen_clear();
}
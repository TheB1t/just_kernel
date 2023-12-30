#include <drivers/early_screen.h>

static screen_char* framebuffer = (screen_char*)0xB8000; 

screen_cursor   cursor          = { .x = 0, .y = 0 };
screen_color    current_color   = { .foreground = 15, .background = 0 };
screen_char     blank	        = { .c = 0x20, .color = { .foreground = 15, .background = 0 } };

screen_color screen_getColor() {
	return current_color;
}

void screen_setColor(screen_color color) {
	current_color = color;
}

static void screen_moveCursor() {
	uint16_t cursor_location = (cursor.y * SCREEN_WIDTH) + cursor.x;
	port_outb(0x3D4, 14);
	port_outb(0x3D5, cursor_location >> 8);
	port_outb(0x3D4, 15);
	port_outb(0x3D5, cursor_location);
}

static void screen_scroll() {
	if (cursor.y >= SCREEN_HEIGHT) {
		for (uint32_t i = 0 * SCREEN_WIDTH; i < (SCREEN_HEIGHT - 1) * SCREEN_WIDTH; i++) {
			framebuffer[i] = framebuffer[i + SCREEN_WIDTH];
		}

		for (uint32_t i = (SCREEN_HEIGHT - 1) * SCREEN_WIDTH; i < SCREEN_HEIGHT * SCREEN_WIDTH; i++) {
			framebuffer[i] = blank;
		}

		cursor.y = SCREEN_HEIGHT - 1;
	}
}

void screen_putChar(char c) {
	switch (c) {
		case '\b':
			if (cursor.x)
				cursor.x--;
				framebuffer[(cursor.y * SCREEN_WIDTH) + cursor.x] = blank;
			break;

		case '\t':
			cursor.x = (cursor.x + (4 - (cursor.x % 4)));// & ~(8 - 1);
			break;

		case '\n':
			cursor.y++;
			
		case '\r':
			cursor.x = 0;
			break;

		default:
			if (c >= ' ') {
				screen_char* sc = &framebuffer[(cursor.y * SCREEN_WIDTH) + cursor.x];
				sc->color = current_color;
				sc->c = c;
				cursor.x++;
			}
	}

	if (cursor.x >= 80) {
		cursor.x = 0;
		cursor.y++;
	}

	screen_scroll();
	screen_moveCursor();
}

void screen_clear() {
	for (uint32_t i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
		framebuffer[i] = blank;
	}

	cursor.x = 0;
	cursor.y = 0;
	screen_moveCursor();
}

void screen_putString(char* c) {
	while (*c) {
		screen_putChar(*c++);
	}
}

void screen_init() {
    _global_putchar = screen_putChar;
}
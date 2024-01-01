#pragma once

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

#include <klibc/stdlib.h>
#include <io/ports.h>

typedef struct {
	uint8_t	x;
	uint8_t y;
} screen_cursor;

typedef struct {
	uint8_t	foreground :4;
	uint8_t background :4;
} screen_color;

typedef struct {
	char c;
	screen_color color;
} screen_char;

screen_color    screen_getColor();
void            screen_setColor(screen_color color);
void            screen_putChar(char c);
void            screen_putString(char* str);
void            screen_clear();
void            screen_init();
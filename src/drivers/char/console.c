#include <drivers/char/tty.h>

static void console_close(tty_t* tty, file_t* file UNUSED) {
	if (!tty)
		return;

    if (tty->count > 1)
        return;
}

static void console_write(tty_t* tty) {
	int32_t c;

	while (!EMPTY(&tty->write_q)) {
		c = get_tty_queue(&tty->write_q);
		serial_putc(c);
	}
}

int32_t console_open(tty_t* tty, file_t* file UNUSED) {
	if (!tty)
		return -ENODEV;

    tty->write = console_write;
	tty->close = console_close;

    return 0;
}
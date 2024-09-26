#include <klibc/stdio.h>
#include <klibc/lock.h>

lock_t kprintf_lock	= INIT_LOCK(kprintf_lock);
printf_putchar_t _global_putchar = NULL;

void _sprintf_putchar(char c, void* arg) {
    char** buf = (char**)arg;

    **buf = c;
    (*buf)++;
	**buf = 0;
}

void sprintf(char* buf, char* format, ...) {
	va_list va;
	va_start(va, format);
	_vprintf(_sprintf_putchar, (void*)&buf, format, va);
	va_end(va);
}

void kprintf(char* format, ...) {
	lock(kprintf_lock);
    va_list va;
    va_start(va, format);
    _vprintf(_global_putchar, NULL, format, va);
    va_end(va);
	unlock(kprintf_lock);
}

void _vprintf(printf_putchar_t putchar, void* putchar_arg, char* format, va_list args) {
	char c, num = 0, sym = ' ';
	char buf[20];
	char padRight = 0;
    uint32_t v = 0;

    if (!putchar) {
        return;
	}

	while ((c = *format++) != 0) {
		padRight = 0;
		sym = ' ';
		num = 0;

		if (c != '%')
			putchar(c, putchar_arg);
		else {
			char* p;

			c = *format++;
		back:
			switch (c) {
				case 'd':
				case 'u':
				case 'x':
                    v = va_arg(args, uint32_t);
					if (c == 'x')
						htoa(v, buf);
					else
						itoa(v, buf);
					p = buf;
					goto string;

				case 's':
					p = va_arg(args, char*);
					if (!p)
						p = "(null)";

				string:
					if (num) {
						num -= (char)strlen(p) > num ? num : (char)strlen(p);
						if (!padRight)
							while (num--)
								putchar(sym, putchar_arg);
					}

					while (*p)
						putchar(*p++, putchar_arg);

					if (num && padRight)
						while (num--)
							putchar(sym, putchar_arg);

					break;

				case 'c':
                    v = va_arg(args, uint32_t);
					putchar((char)v, putchar_arg);
                    break;

				default:
					if (*(format - 2) == '%') {
						if (c == '0' || c == ' ' || c == '.') {
							sym = c;
							c = *format++;
						}
						if (c == '-') {
							padRight = 1;
							c = *format++;
						}
						num = 0;
						while (c >= '1' && c <= '9') {
							num *= 10;
							num += c - '0';
							c = *format++;
						}
						goto back;
					} else {
                        v = va_arg(args, uint32_t);
                        putchar((char)v, putchar_arg);
					}
					break;
			}
		}
	}
}

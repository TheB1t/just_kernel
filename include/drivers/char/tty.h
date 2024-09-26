#pragma once

#include <klibc/stdlib.h>
#include <fs/fs.h>

#define MAX_TTYS                256
#define NR_LDISCS	            16
#define TTY_BUF_SIZE            1024

#define IS_A_CONSOLE(min)	    (((min) & 0b1100000) == 0b0000000)
#define IS_A_SERIAL(min)	    (((min) & 0b1100000) == 0b0100000)
#define IS_A_PTY(min)		    ((min)  & 0b1000000)
#define IS_A_PTY_MASTER(min)	(((min) & 0b1100000) == 0b1000000)
#define IS_A_PTY_SLAVE(min)	    (((min) & 0b1100000) == 0b1100000)
#define PTY_OTHER(min)		    ((min)  ^ 0b0100000)

#define SL_TO_DEV(line)		    ((line) | 0x40)
#define DEV_TO_SL(min)		    ((min)  & 0x3F)

#define INC(a)                  ((a) = ((a) + 1) & (TTY_BUF_SIZE - 1))
#define DEC(a)                  ((a) = ((a) - 1) & (TTY_BUF_SIZE - 1))
#define EMPTY(a)                ((a)->head == (a)->tail)
#define LEFT(a)                 (((a)->tail - (a)->head - 1) & (TTY_BUF_SIZE - 1))
#define LAST(a)                 ((a)->buf[(TTY_BUF_SIZE - 1) & ((a)->head - 1)])
#define FULL(a)                 (!LEFT(a))
#define CHARS(a)                (((a)->head - (a)->tail) & (TTY_BUF_SIZE - 1))

#define TTY_TABLE_IDX(nr)	    ((nr) ? (uint32_t)(nr) : (uint32_t)(fg_console + 1))
#define TTY_TABLE(nr) 		    (tty_table[TTY_TABLE_IDX(nr)])

#define TTY_WRITE_FLUSH(tty)    tty_write_flush((tty))
#define TTY_READ_FLUSH(tty)     tty_read_flush((tty))

typedef struct tty_struct   tty_t;
typedef struct tty_queue    tty_queue_t;
typedef struct tty_ldisc    tty_ldisc_t;

struct tty_queue {
	int32_t data;
	int32_t head;
	int32_t tail;
	uint8_t buf[TTY_BUF_SIZE];
};

struct tty_struct {
    int32_t     count;
	int32_t     disc;
	uint32_t    column;
    uint32_t    line;

	struct {
		uint16_t row;
		uint16_t col;
		uint16_t xpixel;
		uint16_t ypixel;
	} winsize;

    int32_t		(*open)(tty_t* tty, file_t* file);
    void		(*close)(tty_t* tty, file_t* file);
    void		(*write)(tty_t* tty);

    tty_t*      link;

    tty_queue_t write_q;
    tty_queue_t read_q;
};

struct tty_ldisc {
	int32_t	flags;

	int32_t     (*open)(tty_t* tty);
	void    	(*close)(tty_t* tty);
	int32_t     (*read)(tty_t* tty, file_t* file, char* buf, int32_t nr);
	int32_t     (*write)(tty_t* tty, file_t* file, char* buf, int32_t nr);
	void		(*handler)(tty_t* tty);
};

extern tty_t*       tty_table[];
extern tty_ldisc_t  ldiscs[];
extern int32_t      fg_console;

extern void     	tty_write_flush(tty_t* tty);
extern void     	tty_read_flush(tty_t* tty);

extern void     	put_tty_queue(char c, tty_queue_t* queue);
extern int32_t      get_tty_queue(tty_queue_t* queue);

extern int32_t      pty_open(tty_t* tty, file_t* file);
extern int32_t 		console_open(tty_t* tty, file_t* file);
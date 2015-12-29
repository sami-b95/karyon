#ifndef _TTY_H_
#define _TTY_H_

#include <kernel/types.h>

typedef ui32_t tcflag_t;
typedef uchar_t cc_t;
#define NCCS	17

/** termios structure. **/

struct termios
{
	tcflag_t c_iflag;
	tcflag_t c_oflag;
	tcflag_t c_cflag;
	tcflag_t c_lflag;
	cc_t c_cc[NCCS];
};

/** Optionnal actions for tcsetattr. **/

#define TCSANOW		1
#define TCSAFLUSH	3

/** Flush modes. **/

#define TCIFLUSH	1
#define TCIOFLUSH	3

/** c_iflag bits **/

#define INLCR	0000100
#define IGNCR	0000200
#define ICRNL	0000400
#define IXON	0002000
#define IXOFF	0010000

/** c_oflag bits **/

#define OPOST	0000001
#define ONLCR	0000004
#define OCRNL	0000010
#define ONOCR	0000020
#define ONLRET	0000040

/** c_lflag bits **/

#define ISIG	0000001
#define ICANON	0000002
#define ECHO	0000010
#define ECHOE	0000020
#define ECHOK	0000040
#define ECHOCTL	0001000
#define ECHOKE	0004000

/** c_cflag bits **/

#define CSIZE	0000060
#define CS5	0000000
#define CS6	0000020
#define CS7	0000040
#define CS8	0000060
#define CSTOPB	0000100
#define CREAD	0000200
#define PARENB	0000400
#define PARODD	0001000
#define HUPCL	0002000

/* c_cc characters */

#define VINTR		0
#define VQUIT		1
#define VERASE		2
#define VKILL		3
#define VEOF		4
#define VTIME		5
#define VMIN		6
#define VSWTC		7
#define VSTART		8
#define VSTOP		9
#define VSUSP		10
#define VEOL		11
#define VREPRINT	12
#define VDISCARD	13
#define VWERASE		14
#define VLNEXT		15
#define VEOL2		16

/** A macro to determine whether a character marks the end of a line. **/

#define is_eol(c) (c == '\n' \
                || c == termios.c_cc[VEOL] \
                || c == termios.c_cc[VEOL2] \
                || c == termios.c_cc[VEOF])

/** Global variables. **/

#ifdef _TTY_C_
struct termios termios __attribute__((section("DATA"))) = {
	.c_iflag = ICRNL | IXON,
	.c_oflag = OPOST | ONLCR,
	.c_lflag = ICANON | ISIG | ECHO | ECHOK | ECHOKE | ECHOCTL,
	.c_cflag = CS8 | CREAD | HUPCL,
	.c_cc = { 3,
	          28,
	          127,
	          11,
	          4,
	          0,
	          1,
	          255,
	          17,
	          19,
	          26,
	          255,
	          18,
	          15,
	          23,
	          22,
	          255 }
};
struct fifo tty_ififo = {
	.read = 0,
	.write = 0,
	.to_read = 0
};
struct fifo tty_ififo2 = {
	.read = 0,
	.write = 0,
	.to_read = 0
};
pid_t reader_pid = 0;
pid_t pgrp = 2;
count_t eol_count = 0;
#else
extern struct termios termios;
extern struct fifo tty_ififo;
extern struct fifo tty_ififo2;
extern pid_t reader_pid;
extern pid_t pgrp;
extern count_t eol_count;
#endif

/** Functions **/

ssize_t tty_read(uchar_t *buf, size_t size);
/****************************************************************/
ret_t tty_write(uchar_t *buf, size_t size);
/****************************************************************/
ssize_t tty_puts(uchar_t *buf, size_t size);
/****************************************************************/
ret_t tty_process();
/****************************************************************/
void tty_echo(uchar_t c);

#endif

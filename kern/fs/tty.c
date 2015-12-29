/****************************************************************
 * tty.c                                                        *
 *                                                              *
 *    Terminal emulation.                                       *
 *                                                              *
 ****************************************************************/

#define _TTY_C_
#include <config.h> // debugging
#include <fs/fifo.h>
#include <fs/vt100.h>
#include <kernel/errno.h>
#include <kernel/libc.h>
#include <kernel/panic.h>
#include <kernel/process.h>
#include <kernel/screen.h>
#include <kernel/signal.h>

#include "tty.h"

/**
 * tty_read
 */

ssize_t tty_read(uchar_t *buf, size_t size)
{
	uchar_t c;
	size_t i = 0;
	ssize_t read_bytes = 0;

	/** Get the terminal. **/

	while(1)
	{
		sti;
		yield;
		//delay(TTY_DELAY);
		cli;

		if(!reader_pid)
		{
			reader_pid = current_pid;
			break;
		}
	}

	/** Try to read from it. **/

	while(1)
	{
		/** Wait until we can read data. **/

		while(1)
		{
			sti;
			yield;
			//delay(TTY_DELAY);
			cli;

			if(termios.c_lflag & ICANON)
			{
				if(eol_count)
				{
					break;
				}
			}
			else
			{
				if(tty_ififo2.to_read)
				{
					break;
				}
			}
		}

		/** Tranfer the data from the secondary FIFO to the buffer. **/

		while(i < size)
		{
			if(fifo_read_byte(&tty_ififo2, (ui8_t*)&c, 0) != OK)
			{
				break;
			}

			if(is_eol(c))
			{
				eol_count--;
			}

			buf[i++] = c;
			read_bytes++;
		}

		/** Check whether we can exit from the loop and exit if so. **/

		if(termios.c_lflag & ICANON)
		{
			break;
		}
		else if(read_bytes >= termios.c_cc[VMIN])
		{
			break;
		}
	}

	/** Free the terminal. **/

	reader_pid = 0;

	return read_bytes;
}

/**
 * tty_write
 */

ret_t tty_write(uchar_t *buf, size_t size)
{
	size_t i;
	ret_t ret = OK;

	for(i = 0; i < size; i++)
	{
		ret = fifo_write_byte(&tty_ififo, (ui8_t)buf[i], 0);

		if(ret != OK)
		{
			break;
		}
	}

	if(ret == OK)
	{
		ret = tty_process();
	}

	return ret;
}

/**
 * tty_puts
 */

ssize_t tty_puts(uchar_t *buf, size_t size)
{
	size_t i;
	uchar_t c;

	for(i = 0; i < size; i++)
	{
		c = buf[i];

		if(termios.c_oflag & OPOST)
		{
			if(c == '\r' && (termios.c_oflag & OCRNL))
			{
				c = '\n';
			}
			else if(c == '\n' && (termios.c_oflag & ONLRET))
			{
				c = '\r';
			}
			else if(c == '\n' && (termios.c_oflag & ONLCR))
			{
				vt100_put_char('\r');
			}
		}

		vt100_put_char(c);
	}

	return (ssize_t)size;
}

/**
 * tty_process
 */

ret_t tty_process()
{
	uchar_t c, c2;
	ret_t ret;

	while(fifo_read_byte(&tty_ififo, (ui8_t*)&c, 0) == OK)
	{
		if(c == '\r' && (termios.c_iflag & ICRNL))
		{
			c = '\n';
		}

		if(c == '\n' && (termios.c_iflag & INLCR))
		{
			c = '\r';
		}

		/** Perform canonical mode specific operations. **/

		if(termios.c_lflag & ICANON)
		{
			/** Erase a character. **/

			if(c == termios.c_cc[VERASE])
			{
				c2 = tty_ififo2.rbuf[tty_ififo2.write - 1];

				if(!tty_ififo2.to_read || is_eol(c2))
				{
					continue;
				}

				fifo_pop_last(&tty_ififo2);

				if(termios.c_lflag & ECHO)
				{
					if(c2 < 0x20)
					{
						vt100_del();
					}

					vt100_del();
				}

				continue;
			}
			else if(c == termios.c_cc[VKILL])
			{
				while(1)
				{
					c2 = tty_ififo2
					    .rbuf[tty_ififo2.write - 1];

					if(!tty_ififo2.to_read || is_eol(c2))
					{
						break;
					}

					fifo_pop_last(&tty_ififo2);

					if(termios.c_lflag & ECHO)
					{
						if(c2 < 0x20)
						{
							vt100_del();
						}

						vt100_del();
					}
				}

				continue;
			}
		}

		/** If the character is an end of line character, increase
		    eol_count. **/

		if(is_eol(c))
		{
			eol_count++;
		}

		/** Echo to the screen if allowed. **/

		if(termios.c_lflag & ECHO)
		{
			tty_echo(c);
		}

		/** A character can be used to send a signal to the foreground
		    process group. **/

		if(termios.c_lflag & ISIG)
		{
			if(c == termios.c_cc[VINTR])
			{
				signal_send_pgrp(SIGINT, pgrp);
				continue;
			}
			else if(c == termios.c_cc[VSUSP])
			{
				signal_send_pgrp(SIGTSTP, pgrp);
				continue;
			}
		}

		/** Try to write to the secondary FIFO. **/

		ret = fifo_write_byte(&tty_ififo2, (ui8_t)c, 0);

		if(ret != OK)
		{
			return ret;
		}
	}

	return OK;
}

/**
 * tty_echo
 */

void tty_echo(uchar_t c)
{
	if(c == '\n')
	{
		vt100_put_char('\r');
		vt100_put_char('\n');
	}
	else if(c < 0x20)
	{
		if(termios.c_lflag & ECHOCTL)
		{
			vt100_put_char('^');
			vt100_put_char(c + 0x40);
		}
	}
	else
	{
		vt100_put_char(c);
	}
}

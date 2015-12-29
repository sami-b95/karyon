/****************************************************************
 * screen.c                                                     *
 *                                                              *
 *    Basic screen functions.                                   *
 *                                                              *
 ****************************************************************/

#define _SCREEN_C_
#include <kernel/io.h>
#include <kernel/libc.h>
#include <kernel/printk.h>

#include "screen.h"

/**
 * screen_write_char
 */

void screen_write_char(uchar_t c, ui8_t x, ui8_t y)
{
	vram[y * 80 + x] = c | (text_attr << 8);
}

/**
 * screen_print_at
 */

void screen_print_at(uchar_t *s, ui8_t x, ui8_t y)
{
	while(*s)
	{
		screen_write_char(*s++, x, y);
		x++;

		if(x > 79)
		{
			x = 0;
			y++;
		}
	}
}

/**
 * screen_put_char
 */

void screen_put_char(uchar_t c)
{
	if(c == '\n') /* Newline */
	{
		Y++;

		#ifdef SCREEN_NLCR
		X = 0;
		#endif
	}
	else if(c == '\r') /* Carriage return */
	{
		X = 0;
	}
	else if(c == '\b') /* Backspace */
	{
		if(X > 0)
		{
			X--;
		}
		else
		{
			if(Y > 0)
			{
				X = 79;
				Y--;
			}
		}

		//screen_show_cursor();
	}
	else if(c == 0x7f)
	{
		vram[Y * 80 + X] = '\0' | (text_attr << 8);
	}
	else if(c == '\t')
	{
		X = X + 8 - (X % 8);
	}
	else
	{
		vram[Y * 80 + X] = c | (text_attr << 8);
		X++;
	}

	#ifdef PORT_0xE9_HACK
	outb(0xe9, c);
	//delay(500);
	#endif

	if(X > 79)
	{
		X = 0;
		Y++;
	}

	if(Y > 23)
	{
		screen_scroll_up(1);
		Y = 23;
	}

	if(show_cursor)
	{
		//screen_show_cursor();
	}
}

/**
 * screen_print
 */

void screen_print(uchar_t *s)
{
	while(*s)
	{
		screen_put_char(*s++);
	}
}

/**
 * screen_scroll_up
 */

void screen_scroll_up(count_t lines)
{
	ui16_t *from = vram + lines * 80,
	       *to = vram,
	       *end = vram + 24 * 80;

	while(from < end)
	{
		*to++ = *from++;
	}

	while(to < end)
	{
		*to++ = '\0' | (text_attr << 8);
	}
}

/**
 * screen_clear
 */

void screen_clear()
{
	screen_scroll_up(24);
	X = Y = 0;
}

/**
 * screen_move_cursor
 */

void screen_move_cursor(ui8_t x, ui8_t y)
{
	ui16_t pos = y * 80 + x;

	outb(0x3d4, 0x0f);
	outb(0x3d5, (ui8_t)pos);
	outb(0x3d4, 0x0e);
	outb(0x3d5, (ui8_t)(pos >> 8));
}

/**
 * screen_show_cursor
 */

void screen_show_cursor()
{
	screen_move_cursor(X, Y);
}

/**
 * screen_hide_cursor
 */

void screen_hide_cursor()
{
	screen_move_cursor(-1, -1);
}

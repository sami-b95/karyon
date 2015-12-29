#include <fs/tty.h>
#include <kernel/io.h>
#include <kernel/libc.h>
#include <kernel/panic.h>
#include <kernel/screen.h>
#include <config.h>

#include "vt100.h"

static enum {
	VT100_NORMAL = 1,
	VT100_ESC = 2, // got \033
	VT100_CSI = 3, // got \033[
	VT100_PARAM = 4, // getting parameters
	VT100_CSI_CMD = 5 // processing command with CSI
} state = VT100_NORMAL;
static ui8_t saved_X, saved_Y;
static ui8_t first_line = 0, last_line = 23;
static ui16_t opt = VT100_WRAP;
static ui8_t param[MAX_PARAM], param_i = 0;
bool_t ques_mark = 0, open_par = 0, close_par = 0;

/**
 * vt100_putchar
 */

void vt100_put_char(uchar_t c)
{
	ui8_t i;

	#ifdef PORT_0xE9_HACK
	outb(0xe9, c);
	#endif

	/*if(X >= 80)
	{
		X -= 80;					
	}

	vram[Y * 80 + X] = c | (text_attr << 8);

	if(opt & VT100_WRAP)
	{
		X++;

		if(X >= 80)
		{
			vt100_nl();
		}
	}
	else if(X < 79)
	{
		X++;
	}

	return;*/

	if(state == VT100_NORMAL)
	{
		switch(c)
		{
			case '\033':
				state = VT100_ESC;
				goto end;

			case '\b':
				if(X)
				{
					X--;
				}
				break;

			case '\r':
				X = 0;
				break;

			case '\n':
				vt100_nl();
				break;

			case '\t':
				X = X + 8 - (X % 8);
				break;

			case 0x0e: case 0x0f: // shift out/shift in
				break;

			case 0x7f:
				vt100_del();
				break;

			default:
				if(X >= 80)
				{
					X -= 80;					
				}

				vram[Y * 80 + X] = c | (text_attr << 8);

				if(opt & VT100_WRAP)
				{
					X++;

					if(X >= 80)
					{
						vt100_nl();
					}
				}
				else if(X < 79)
				{
					//X++;
				}

				break;
		}
	}

	if(state == VT100_ESC)
	{
		switch(c)
		{
			case '(':
				open_par = 1;
				goto end;

			case ')':
				close_par = 1;
				goto end;

			case '=':
				break;

			case '[': // CSI
				state = VT100_CSI;
				goto end;

			case '7': // save cursor
				vt100_cursor_save();
				break;

			case '8': // restore cursor
				vt100_cursor_restore();
				break;

			case 'c': // reset terminal
				text_attr = DEF_TEXT_ATTR;				
				first_line = 0;
				last_line = 23;
				opt = VT100_WRAP;
				param_i = 0;
				state = VT100_NORMAL;
				break;

			case 'r': // debug: display scrolling region
				printk("%x, %x\n", first_line, last_line);
				break;

			case 'B': case '0':
				if(open_par || close_par) // TODO
				{
					open_par = close_par = 0;
				}
				break;

			case 'D': // new line
				vt100_nl();
				break;

			case 'H': // TODO: set a tab
				break;

			case 'M': // cursor up, scroll down if at the top
				if(Y > first_line)
				{
					Y--;
				}
				else
				{
					vt100_scroll_down();
				}
				break;

			default:
				//panic("unrecognised NON-CSI command %c", c); 
				break;
		}

		state = VT100_NORMAL;
	}

	if(state == VT100_CSI)
	{
		for(i = 0; i < MAX_PARAM; i++)
		{
			param[i] = 0;
		}

		param_i = 0;

		if(c >= '0' && c <= '9')
		{
			state = VT100_PARAM;
		}
		else if(c == '?')
		{
			ques_mark = 1;
			goto end;
		}
		else
		{
			state = VT100_CSI_CMD;
		}
	}

	if(state == VT100_PARAM)
	{
		if(param_i >= MAX_PARAM)
		{
			state = VT100_NORMAL;
		}

		if(c >= '0' && c <= '9')
		{
			param[param_i] = param[param_i] * 10 + c - '0';
		}
		else if(c == ';' && (param_i + 1) < MAX_PARAM)
		{
			param_i++;
		}
		else
		{
			state = VT100_CSI_CMD;
		}
	}

	if(state == VT100_CSI_CMD)
	{
		switch(c)
		{
			case 'c': // report device status
				tty_write("\033[?1;2c", 7);
				break;

			case 'g': // TODO
				if(!param[0]) // clear tab
				{
				}
				else if(param[0] == 3) // clr all tabs
				{
				}
				break;

			case 'h':
				if(ques_mark) // TODO
				{
					ques_mark = 0;
				}
				else if(param[0] == 7) // enable line wrap
				{
					opt |= VT100_WRAP;
				}
				break;

			case 'l':
				if(ques_mark) // TODO
				{
					ques_mark = 0;
				}
				else if(param[0] == 7) // disable line wrap
				{
					opt &= ~VT100_WRAP;
				}
				break;

			case 'm': // set text attr
				for(i = 0; i <= param_i; i++)
				{
					if(param[i] == 0) // reset
					{
						text_attr = DEF_TEXT_ATTR;
					}
					else if(param[i] == 1) // bright
					{
						text_attr |= 8;
					}
					else if(param[i] == 5) // blink
					{
						text_attr |= 0x80;
					}
					else if(param[i] == 7) // reverse
					{
						ui8_t
						   bright = text_attr & 0x08,
						   blink = text_attr & 0x80;

						text_attr = ((text_attr >> 4)
							   | (text_attr << 4))
						            & 0x77;
						text_attr |= (bright | blink);
					}
				}
				break;

			case 'n': // query
				if(param[0] == 5) // device status
				{
					tty_write("\033[0n", 4);
				}
				else if(param[0] == 6) // cursor position
				{
					uchar_t s[8];

					// FIXME: UGLY!!!

					snprintf(s, 8, "\033[%c%c;%c%cR",
					         '0' + (Y + 1) / 10,
					         '0' + ((Y + 1) % 10),
					         '0' + (X + 1) / 10,
					         '0' + ((X + 1) % 10));
					state = VT100_NORMAL;
					tty_write(s, 8);
				}
				break;
				

			case 'r': // set scrolling region
				first_line = param[0] ? param[0] - 1 : 0;
				last_line = param[1] ? param[1] - 1: 23;
				break;

			case 's': // save cursor
				vt100_cursor_save();
				break;

			case 'u': // restore cursor
				vt100_cursor_restore();
				break;

			case 'A': // cursor up
				vt100_cursor_move(X, Y - (param[0]
				                          ? param[0] : 1));
				break;

			case 'B': // cursor down
				vt100_cursor_move(X, Y + (param[0]
				                        ? param[0] : 1));
				break;

			case 'C': // cursor right
				vt100_cursor_move(X + (param[0]
				                     ? param[0] : 1), Y);
				break;

			case 'D': // cursor left
				vt100_cursor_move(X - (param[0]
				                     ? param[0] : 1), Y);
				break;

			case 'H': // set cursor position
				vt100_cursor_move(param[1] ? param[1] - 1 : 0,
				                  param[0] ? param[0] - 1 : 0);
				break;

			case 'J': // clear screen
				switch(param[0])
				{
					case 0:
						vt100_clear_region(Y, 23);
						break;

					case 1:
						vt100_clear_region(0, Y);
						break;

					case 2:
						vt100_clear_region(0, 23);
						vt100_cursor_move(0, 0);
						break;

					default:
						break;
				}
				break;

			case 'K': // clear line
				switch(param[0])
				{
					case 0:
						vt100_clear_line(X, 79);
						break;

					case 1:
						vt100_clear_line(0, X);
						break;

					case 2:
						vt100_clear_line(0, 79);
						break;

					default:
						break;
				}
				break;

			default:
				//panic("unrecognised CSI command %c", c); 
				break;
		}

		state = VT100_NORMAL;
	}

	end:
		screen_show_cursor();
}

/**
 * vt100_cursor_save
 */

void vt100_cursor_save()
{
	saved_X = X;
	saved_Y = Y;
}

/**
 * vt100_cursor_restore
 */

void vt100_cursor_restore()
{
	X = saved_X;
	Y = saved_Y;
}

/**
 * vt100_cursor_move
 */

void vt100_cursor_move(si8_t new_X, si8_t new_Y)
{
	if(new_X < 0)
	{
		X = 0;
	}
	else if(new_X >= 80)
	{
		X = 79;
	}
	else
	{
		X = new_X;
	}

	if(new_Y < 0)
	{
		Y = 0;
	}
	else if(new_Y >= 24)
	{
		Y = 23;
	}
	else
	{
		Y = new_Y;
	}
}

/**
 * vt100_scroll_up
 */

void vt100_scroll_up()
{
	ui16_t *p = (ui16_t*)0xb8000 + first_line * 80;

	while(p < (ui16_t*)0xb8000 + (last_line + 1) * 80)
	{
		if(p < (ui16_t*)0xb8000 + last_line * 80)
		{
			*p = *(p + 80);
		}
		else
		{
			*p = ' ' | (text_attr << 8);
		}

		p++;
	}
}

/**
 * vt100_scroll_down
 */

void vt100_scroll_down()
{
	ui16_t *p = (ui16_t*)0xb8000 + last_line * 80 + 79;

	while(p >= (ui16_t*)0xb8000 + first_line * 80)
	{
		if(p < (ui16_t*)0xb8000 + (first_line + 1) * 80)
		{
			*p = ' ' | (text_attr << 8);
		}
		else
		{
			*p = *(p - 80);
		}

		p--;
	}
}

/**
 * vt100_clear_region
 */

void vt100_clear_region(si8_t top, si8_t bottom)
{
	si8_t count;
	ui8_t saved_first_line = first_line, saved_last_line = last_line;
	
	first_line = top;
	last_line = bottom;

	count = top - bottom + 1;

	while(count--)
	{
		vt100_scroll_up();
	}

	first_line = saved_first_line;
	last_line = saved_last_line;
}

/**
 * vt100_clear_line
 */

void vt100_clear_line(si8_t xmin, si8_t xmax)
{
	si8_t x = xmin;

	while(x <= xmax)
	{
		vram[Y * 80 + x] = ' ' | (text_attr << 8);
		x++;
	}
}

/**
 * vt100_nl
 */

void vt100_nl()
{
	if(Y < last_line)
	{
		Y++;
	}
	else
	{
		vt100_scroll_up();
	}
}

/**
 * vt100_del
 */

void vt100_del()
{
	if(X)
	{
		X--;
		vram[Y * 80 + X] = ' ' | (text_attr << 8);
	}

	screen_show_cursor();
}

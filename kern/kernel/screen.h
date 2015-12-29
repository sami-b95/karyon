#ifndef _SCREEN_H_
#define _SCREEN_H_

#include <config.h>
#include <kernel/types.h>

/** Global text attribute **/

#ifdef _SCREEN_C_
ui16_t *vram = (ui16_t*)0xb8000;
si8_t X, Y; // WARNING: These must be signed (cf. VT100 emulation)
ui8_t text_attr = DEF_TEXT_ATTR;
bool_t show_cursor = 0;
#else
extern ui16_t *vram;
extern ui8_t X, Y;
extern ui8_t text_attr;
extern bool_t show_cursor;
#endif

/** Functions **/

void screen_write_char(uchar_t c, ui8_t x, ui8_t y);
/****************************************************************/
void screen_print_at(uchar_t *s, ui8_t x, ui8_t y);
/****************************************************************/
void screen_put_char(uchar_t c);
/****************************************************************/
void screen_print(uchar_t *s);
/****************************************************************/
void screen_scroll_up(count_t lines);
/****************************************************************/
void screen_clear();
/****************************************************************/
void screen_move_cursor(ui8_t x, ui8_t y);
/****************************************************************/
void screen_show_cursor();
/****************************************************************/
void screen_hide_cursor();

#endif

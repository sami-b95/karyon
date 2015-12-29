#ifndef _LIBC_H_
#define _LIBC_H_

#include <kernel/stdarg.h>
#include <kernel/types.h>

/** Macros. **/

#define abs(x)			((x) > 0 ? (x) : -(x))
#define min(x, y)		(((x) < y) ? (x) : (y))
#define byte_in_win(byte, win_start, win_end) \
	((byte) - (win_start) <= (win_end) - (win_start))
#define seg_inter_win(seg_start, seg_end, win_start, win_end) \
	  (((win_start) == (win_end + 1) \
	    && (seg_start) == (seg_end + 1) \
	    && (seg_start) == (win_start)) \
	|| ((win_start) != (win_end + 1) \
	    && (byte_in_win(seg_start, win_start, win_end) \
	     || byte_in_win(seg_end, win_start, win_end))))
#define seg_in_win(seg_start, seg_end, win_start, win_end) \
	(byte_in_win(seg_start, win_start, win_end) \
	&& byte_in_win(seg_end, win_start, win_end))
#define mod2pow32_compare(x, y) \
	  (((y) - (x) + 0x80000000 > 0x80000000) ? -1 : ((x == y) ? 0 : 1))

/** Functions **/

void *memcpy(void *dest, void *src, size_t n);
/****************************************************************/
si32_t memcmp(const void *s1, const void *s2, size_t n);
/****************************************************************/
void *memset(void *dest, ui8_t val, size_t n);
/****************************************************************/
uchar_t *strncpy(uchar_t *dest, uchar_t *src, size_t n);
/****************************************************************/
uchar_t *strcpy(uchar_t *dest, uchar_t *src);
/****************************************************************/
size_t strlen(const uchar_t *s);
/****************************************************************/
si32_t strncmp(const uchar_t *s1, const uchar_t *s2, size_t n);
/****************************************************************/
uchar_t *strchr(const uchar_t *s, uchar_t c);
/****************************************************************/
count_t vsnprintf(uchar_t *s, size_t size, uchar_t *f, va_list ap);
/****************************************************************/
count_t snprintf(uchar_t *s, size_t n, uchar_t *f, ...);
/****************************************************************/
inline void delay(ui32_t n);

#endif

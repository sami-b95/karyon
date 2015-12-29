/****************************************************************
 * libc.c                                                       *
 *                                                              *
 *    Basic C library for kernel.                               *
 *                                                              *
 ****************************************************************/

#include "libc.h"

/**
 * memcpy
 */

void *memcpy(void *dest, void *src, size_t n)
{
	size_t i;

	for(i = 0; i < n; i++)
	{
		((ui8_t*)dest)[i] = ((ui8_t*)src)[i];
	}

	return dest;
}

/**
 * memcmp
 */

si32_t memcmp(const void *s1, const void *s2, size_t n)
{
	size_t i;

	for(i = 0; i < n && ((ui8_t*)s1)[i] == ((ui8_t*)s2)[i]; i++);

	return (i == n) ? 0 : ((((ui8_t*)s1)[i] < ((ui8_t*)s2)[i]) ? -1 : 1);
}

/**
 * memset
 */

void *memset(void *dest, ui8_t val, size_t n)
{
	size_t i;

	for(i = 0; i < n; i++)
	{
		((ui8_t*)dest)[i] = val;
	}

	return dest;
}

/**
 * strncpy
 */

uchar_t *strncpy(uchar_t *dest, uchar_t *src, size_t n)
{
	size_t i;

	for(i = 0; i < n && src[i]; i++)
	{
		dest[i] = src[i];
	}

	for(; i < n; i++)
	{
		dest[i] = '\0';
	}

	return dest;
}

/**
 * strcpy
 */

uchar_t *strcpy(uchar_t *dest, uchar_t *src)
{
	while(*src)
	{
		*(dest++) = *(src++);
	}

	*dest = '\0';

	return dest;
}	

/**
 * strlen
 */

size_t strlen(const uchar_t *s)
{
	size_t len = 0;

	while(*s++)
	{
		len++;
	}

	return len;
}

/**
 * strncmp
 */

si32_t strncmp(const uchar_t *s1, const uchar_t *s2, size_t n)
{
	ui32_t i;

	for(i = 0; i < n && s1[i] == s2[i]; i++);

	return (i == n) ? 0 : ((s1[i] < s2[i]) ? -1 : 1);
}

/**
 * strchr
 */

uchar_t *strchr(const uchar_t *s, uchar_t c)
{
	ui32_t i;

	for(i = 0; s[i]; i++)
	{
		if(s[i] == c)
		{
			return (uchar_t*)s + i;
		}
	}

	return 0;
}

/**
 * vsnprintf
 */

count_t vsnprintf(uchar_t *s, size_t size, uchar_t *f, va_list ap)
{
	size_t count = 0;
	ui32_t int_arg;
	uchar_t char_arg;
	uchar_t *string_arg; size_t string_len;
	static uchar_t *digits = "0123456789abcdef";

	while(*f && count < size)
	{
		if(*f == '%')
		{
			f++;

			if(*f == '%')
			{
				*s++ = '%';
				count++;
			}
			else if(*f == 'x' || *f == 'p')
			{
				int_arg = va_arg(ap, ui32_t);
				*s++ = digits[int_arg >> 28];
				*s++ = digits[int_arg >> 24 & 0xf];
				*s++ = digits[int_arg >> 20 & 0xf];
				*s++ = digits[int_arg >> 16 & 0xf];
				*s++ = digits[int_arg >> 12 & 0xf];
				*s++ = digits[int_arg >> 8 & 0xf];
				*s++ = digits[int_arg >> 4 & 0xf];
				*s++ = digits[int_arg & 0xf];
				count += 8;
			}
			else if(*f == 'c')
			{
				char_arg = (uchar_t)va_arg(ap, ui32_t);
				*s++ = char_arg;
				count++;
			}
			else if(*f == 's')
			{
				string_arg = va_arg(ap, uchar_t*);
				strncpy(s, string_arg, size - count);
				string_len = strlen(string_arg);

				if(string_len > size - count)
				{
					string_len = size - count;
				}

				s += string_len;
				count += string_len;
			}
		}
		else
		{
			*s++ = *f;
			count++;
		}

		f++;
	}

	*s = '\0';

	return count;
}

/**
 * snprintf
 */

count_t snprintf(uchar_t *s, size_t n, uchar_t *f, ...)
{
	count_t ret;
	va_list ap;

	va_start(ap, f);
	ret = vsnprintf(s, n, f, ap);
	va_end(ap);

	return ret;
}

/**
 * delay
 */

inline void delay(ui32_t n)
{
	ui32_t N = n * 1000;

	while(N--);
}

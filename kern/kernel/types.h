#ifndef _TYPES_H_
#define _TYPES_H_

/* Type definitions */

typedef char si8_t;
typedef short si16_t;
typedef long si32_t;
typedef long long si64_t;
typedef unsigned char ui8_t;
typedef unsigned short ui16_t;
typedef unsigned long ui32_t;
typedef unsigned long long ui64_t;

typedef si32_t ssize_t;
typedef ui32_t size_t;
typedef si32_t pid_t;
typedef ui32_t off_t;
typedef ui32_t ino_t;
typedef ui32_t time_t;
typedef ui32_t clock_t;
typedef ui32_t mode_t;
typedef ui32_t socklen_t;

struct timeval
{
	time_t tv_sec;
	si32_t tv_usec;
};

struct rusage
{
	struct timeval ru_utime;
	struct timeval ru_stime;
};

typedef ui32_t count_t;
typedef int ret_t;
typedef int bool_t;
typedef char uchar_t;

#endif

#ifndef _ERRNO_H_
#define _ERRNO_H_

#include <kernel/types.h>

#define OK		0
#define EPERM		1
#define ENOENT		2
#define EINTR		4
#define EIO		5
#define ENXIO		6
#define EBADF		9
#define ECHILD		10
#define EAGAIN		11
#define EWOULDBLOCK	EAGAIN
#define ENOMEM		12
#define EFAULT		14
#define EBUSY		16
#define EEXIST		17
#define ENOTDIR		20
#define EISDIR		21
#define EINVAL		22
#define ENFILE		23
#define EMFILE		24
#define ENOSPC		28
#define EPIPE		32
#define ENOSYS		88
#define ENOTEMPTY	90
#define ENAMETOOLONG	91
#define ECONNRESET	104
#define ENOBUFS		105
#define ENOTSOCK	108
#define EADDRINUSE	112
#define ETIMEDOUT	116
#define EINPROGRESS	119
#define EALREADY	120
#define EMSGSIZE	122
#define EISCONN		127
#define ENOTCONN	128
#define EOVERFLOW	139

/** Pointer to errno. **/

#ifdef _SYSCALL_C_
ui32_t dummy_errno = 0;
#else
extern ui32_t dummy_errno;
#endif

#endif

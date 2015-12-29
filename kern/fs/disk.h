#ifndef _DISK_H_
#define _DISK_H_

#include <kernel/types.h>

/** Functions. **/

ret_t disk_read(void *buf, size_t size, off_t off);
/****************************************************************/
ret_t disk_write(void *buf, size_t size, off_t off);

#endif

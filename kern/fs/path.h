#ifndef _PATH_H_
#define _PATH_H_

#include <config.h>
#include <fs/ext2.h>
#include <kernel/types.h>

/** Global path string. **/

#ifdef _PATH_C_
uchar_t tmp_path[PATH_MAX_LEN + 1];
#else
extern uchar_t tmp_path[];
#endif

/** Functions. **/

ret_t path_clean(uchar_t *path);
/****************************************************************/
ret_t path_to_file(uchar_t *clean_path,
                   ui32_t *file_inum,
                   ui32_t *dir_inum,
                   uchar_t *file_name);
/****************************************************************/
ret_t path_create(uchar_t *clean_path,
                  bool_t is_dir,
                  ui32_t link_to,
                  struct ext2_file *file);

#endif

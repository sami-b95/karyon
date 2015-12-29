/****************************************************************
 * path.c                                                       *
 *                                                              *
 *    Helper functions to manage paths.                         *
 *                                                              *
 ****************************************************************/

#define _PATH_C_
#include <config.h> // debugging
#include <fs/ext2.h>
#include <kernel/errno.h>
#include <kernel/libc.h>
#include <kernel/panic.h>
#include <kernel/printk.h>
#include <kernel/process.h>

#include "path.h"

uchar_t tmp_name[NAME_MAX_LEN + 1];
uchar_t tmp_name2[NAME_MAX_LEN + 1];
uchar_t tmp_path2[PATH_MAX_LEN + 1];

/**
 * path_clean
 */

ret_t path_clean(uchar_t *path)
{
	size_t cwd_len = strlen(current->cwd),
	       path_len = strlen(path);
	uchar_t *p;

	/** Check whether the path is too long. **/

	if((path[0] == '/' && path_len >= PATH_MAX_LEN)
	|| (path[0] != '/' && (cwd_len + path_len + 1) >= PATH_MAX_LEN))
	{
		return -EINVAL;
	}

	/** If the path is relative, convert it to an absolute path. **/

	if(path[0] != '/')
	{
		strncpy(tmp_path2, current->cwd, cwd_len);
		tmp_path2[cwd_len] = '/';
		strncpy(tmp_path2 + cwd_len + 1, path, PATH_MAX_LEN - cwd_len);
			// add a null character at the end of the string
		strncpy(path, tmp_path2, PATH_MAX_LEN + 1);
		path_len = strlen(path);
	}

	/** Get rid of ".", ".." and of excedentary slashes. **/

	enum { NORMAL = 0, SLASH = 1, POINT1 = 2, POINT2 = 3 } state = NORMAL;

	p = path;

	/*printk("--------------------------------------\n");
	printk("initial path is \"%s\"\n", path);*/

	do
	{
		switch(*p)
		{
			case '/':
				if(state == NORMAL)
				{
					state = SLASH;
					p++;
				}
				else if(state == SLASH)
				{
					strcpy(p - 1, p);
				}

				break;

			case '.':
				if(state == SLASH)
				{
					state = POINT1;
				}
				else if(state == POINT1)
				{
					state = POINT2;
				}
				else
				{
					state = NORMAL;
				}

				p++;
				break;

			case '\0':
				break;

			default:
				/** Necessary: for instance, "/..myfile" **/
				state = NORMAL;
				p++;
				break;
		}

		if(*p == '/' || *p == '\0')
		{
			if(state == POINT1)
			{
				if(p - 2 < path)
				{
					//panic("bad path");
					return -EINVAL;
				}

				strcpy(p - 2, p);
				p -= 2;
				state = NORMAL;
			}
			else if(state == POINT2)
			{
				uchar_t *p2 = p - 4;

				if(p2 < path)
				{
					p2 = p - 3;
				}
				else
				{
					while(*p2 != '/')
					{
						p2--;
					}
				}

				strcpy(p2, p);
				p = p2;
				state = NORMAL;
			}
		}

		/*printk("path: \"%s\" | p: \"%s\" | state: %x\n",
		       path, p, state);*/
	} while(*p);

	if(state == SLASH)
	{
		*(p - 1) = '\0';
	}

	return OK;
}

/**
 * path_to_file
 */

ret_t path_to_file(uchar_t *clean_path,
                   ui32_t *file_inum,
                   ui32_t *dir_inum,
                   uchar_t *file_name)
{
	uchar_t *beg_p, *end_p;
	size_t name_len;
	ret_t ret;
	static struct ext2_dent_info dent_info;
	ui32_t parent, son = 2;
	static struct ext2_inode inode;

	end_p = clean_path;

	while(*end_p)
	{
		parent = son;
		beg_p = end_p + 1;
		end_p++;

		while(*end_p != '/' && *end_p != '\0')
		{
			end_p++;
		}

		if((end_p - beg_p) >= NAME_MAX_LEN)
		{
			return -EINVAL;
		}

		name_len = (size_t)(end_p - beg_p);
		strncpy(tmp_name, beg_p, name_len);
		tmp_name[name_len] = '\0';
		/*#ifdef DEBUG
		printk("looking for %s in %s\n",
		       tmp_name,
		       parent.dent.file_name);
		#endif*/

		ret = ext2_find_dent(parent, tmp_name, &dent_info);

		if(ret != OK)
		{
			return ret;
		}

		if(dir_inum)
		{
			*dir_inum = parent;
		}

		son = dent_info.dent.d_inode;

		if((ret = ext2_read_inode(son, &inode)) != OK)
		{
			return ret;
		}

		if(!(inode.i_type_perm & EXT2_DIR) && *end_p == '/')
		{
			return -EINVAL;
		}
	}

	if(file_inum)
	{
		*file_inum = son;
	}

	if(file_name)
	{
		strncpy(file_name, tmp_name, NAME_MAX_LEN + 1);
	}

	return OK;
}

/**
 * path_create
 */

/* WARNING: Before calling this function, one must check that the path doesn't
   exist already. Indeed, this function doesn't handle the case where the path
   exists and the file the path refers to is in the file table. */

ret_t path_create(uchar_t *clean_path,
                  bool_t is_dir,
                  ui32_t link_to,
                  struct ext2_file *file)
{
	uchar_t *beg_p, *end_p;
	size_t name_len;
	ret_t ret;
	struct ext2_dent_info dent_info;
	struct ext2_file son[2];
	struct ext2_file *pparent, *pson;
	int i = 1;

	//printk("creating %s (%x)\n", clean_path, is_dir);

	son[0].inum = 2;

	if((ret = file_fetch(FS_EXT2, &son[0].inum, &son[0], (void**)&pson, 0))
	   != OK)
	{
		return ret;
	}

	end_p = clean_path;

	while(*end_p)
	{
		pparent = pson;
		beg_p = end_p + 1;
		end_p++;

		while(*end_p != '/' && *end_p != '\0')
		{
			end_p++;
		}

		if((end_p - beg_p) >= NAME_MAX_LEN)
		{
			return -EINVAL;
		}

		name_len = (size_t)(end_p - beg_p);
		strncpy(tmp_name, beg_p, name_len);
		tmp_name[name_len] = '\0';
		ret = ext2_find_dent(pparent->inum, tmp_name, &dent_info);

		if(ret == OK)
		{
			if((ret = file_fetch(FS_EXT2,
			                     &dent_info.dent.d_inode,
			                     &son[i++ % 2],
			                     (void**)&pson,
			                     0)) != OK)
			{
				return ret;
			}
		}
		else if(ret == -ENOENT)
		{
			if(*end_p == '/')
			{
				#ifdef DEBUG
				printk("creating dir \"%s\" in %x\n",
				       tmp_name, parent_inum);
				#endif

				if((ret = ext2_create(pparent,
				                      EXT2_DIR,
				                      &son[i % 2])) != OK)
				{
					return ret;
				}

				pson = &son[i++ % 2];
				ret = ext2_link(pparent, pson, tmp_name);

				if(ret != OK)
				{
					if(ext2_remove(pson->inum) != OK)
					{
						panic("can't rm dir");
					}

					return ret;
				}
			}
			else if(*end_p == '\0')
			{
				#ifdef DEBUG
				printk("creating file \"%s\" in %x\n",
				       tmp_name, pparent->inum);
				#endif

				if(link_to)
				{
					if((ret = file_fetch(FS_EXT2,
					                     &link_to,
					                     &son[i++ % 2],
					                     (void**)&pson,
					                     0)) != OK)
					{
						return ret;
					}
				}
				else
				{
					if((ret = ext2_create(pparent,
					                      is_dir
					                    ? EXT2_DIR
					                    : EXT2_REG_FILE,
					                      &son[i % 2]))
					   != OK)
					{
						return ret;
					}

					/*printk("after: pparent->inum: %x\n",
					       pparent->inum);*/
					pson = &son[i++ % 2];
				}

				/*printk("after 2: pparent->inum: %x\n",
				       pparent->inum);*/
				ret = ext2_link(pparent, pson, tmp_name);

				if(ret != OK)
				{
					if(!link_to)
					{
						if(ext2_remove(pson->inum) != OK)
						{
							panic("can't rm file");
						}
					}

					return ret;
				}
			}
			else
			{
				panic("bad *endp value");
			}
		}
		else
		{
			//printk("returned error %x\n", -ret);
			return ret;
		}
	}

	*file = *pson;

	return OK;
}

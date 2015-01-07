/* Memory management routine
   Copyright (C) 1998 Kunihiro Ishiguro

This file is part of GNU Zebra.

GNU Zebra is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

GNU Zebra is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Zebra; see the file COPYING.  If not, write to the Free
Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.  */

#ifndef _ZEBRA_MEMORY_H
#define _ZEBRA_MEMORY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <stdarg.h>
/* #define MEMORY_LOG */

/* For tagging memory, below is the type of the memory. */
enum
{
	MTYPE_TMP = 1,
	MTYPE_STRVEC,
	MTYPE_VECTOR,
	MTYPE_VECTOR_INDEX,
	MTYPE_LINK_LIST,
	MTYPE_LINK_NODE,
	MTYPE_DESC,
	MTYPE_THREAD,
	MTYPE_THREAD_MASTER,
	MTYPE_VTY,
	MTYPE_VTY_OUT_BUF,
	MTYPE_BUFFER,
	MTYPE_BUFFER_DATA,
	MTYPE_STREAM,
	MTYPE_STREAM_DATA,
	MTYPE_STREAM_FIFO,
	MTYPE_PREFIX,
	MTYPE_PREFIX_IPV4,
	MTYPE_PREFIX_IPV6,
	MTYPE_HASH,
	MTYPE_HASH_INDEX,
	MTYPE_HASH_BACKET,

	MTYPE_VTYSH_CONFIG,
	MTYPE_VTYSH_CONFIG_LINE,
	MTYPE_SLABLIST,
	MTYPE_NICSTAT,

	MTYPE_MODINFO,
	MTYPE_DPINFO,
    MTYPE_VERSION,
    MTYPE_VERSION_NAME,
    MTYPE_PROJECT,
    MTYPE_PROJECT_NAME,
    MTYPE_PROJECT_INSTALLNAME,
    MTYPE_MODULE,
    MTYPE_MODULE_NAME,
    MTYPE_MODULE_INSTALLNAME,

	MTYPE_MAX
};


#define XMALLOC(mtype, size)       zmalloc ((mtype), (size))
#define XCALLOC(mtype, size)       zcalloc ((mtype), (size))
#define XREALLOC(mtype, ptr, size) zrealloc ((mtype), (ptr), (size))
#define XFREE(mtype, ptr)          zfree ((mtype), (ptr))
#define XSTRDUP(mtype, str)        zstrdup ((mtype), (str))

/* Prototypes of memory function. */
void *zmalloc (int type, size_t size);
void *zcalloc (int type, size_t size);
void *zrealloc (int type, void *ptr, size_t size);
void  zfree (int type, void *ptr);
char *zstrdup (int type, char *str);
#endif /* _ZEBRA_MEMORY_H */

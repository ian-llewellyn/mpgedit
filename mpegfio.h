/*
 * mpgedit file I/O and buffering routines header
 *
 * Copyright (C) 2001 Adam Bernstein
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307  USA.
 */

/*
 * SccsId[] = "$Id: mpegfio.h,v 1.7 2004/11/27 21:13:29 number6 Exp $"
 */

#ifndef _MPEGFIO_H
#define _MPEGFIO_H

#include <stdio.h>
#include "header.h"
#include "xing_header.h"

typedef struct _mpeg_file_iobuf
{
    unsigned char buf[4096];
    int len;
    int start;
} mpeg_file_iobuf;

#define mpeg_file_iobuf_clear(b)       (b)->len = 0; (b)->start = 0
#define mpeg_file_iobuf_getptr(b)      ((b)->buf + (b)->start)
#define mpeg_file_iobuf_setstart(b, s) ((b)->start = (s))
#define mpeg_file_iobuf_getstart(b)    ((b)->start)
#if 1
#define mpeg_file_iobuf_getlen(b)      ((b)->len - (b)->start)
#define mpeg_file_iobuf_buflen(b)      (sizeof((b)->buf))
#endif

#ifndef mpeg_file_iobuf_clear
void mpeg_file_iobuf_clear(mpeg_file_iobuf *buf);
#endif
#ifndef mpeg_file_iobuf_getptr
unsigned char *mpeg_file_iobuf_getptr(mpeg_file_iobuf *buf);
#endif
#ifndef mpeg_file_iobuf_setstart
void mpeg_file_iobuf_setstart(mpeg_file_iobuf *buf, int start);
#endif
#ifndef mpeg_file_iobuf_getstart
int mpeg_file_iobuf_getstart(mpeg_file_iobuf *buf);
#endif
#ifndef mpeg_file_iobuf_getlen
int mpeg_file_iobuf_getlen(mpeg_file_iobuf *buf);
#endif
#ifndef mpeg_file_iobuf_buflen
int mpeg_file_iobuf_buflen(mpeg_file_iobuf *buf);
#endif

int mpeg_file_iobuf_read(FILE *fp, mpeg_file_iobuf *buf);

int read_next_mpeg_frame(FILE *fp,
                         mpeg_file_iobuf *mpegiobuf,
                         mpeg_header_data *header,
                         int *eof);

int mpegfio_has_xing_header(FILE *fp,
                            mpeg_header_data *mpeg_header,
                            XHEADDATA *xingh, int append);

/*
 * Exported API.
 */
_DSOEXPORT FILE * _CDECL mpeg_file_open(char *file, char *mode);

_DSOEXPORT void _CDECL mpeg_file_close(FILE *fp);

_DSOEXPORT int _CDECL mpeg_file_next_frame_read(FILE *fp,
                                                mpeg_file_iobuf *mpegiobuf,
                                                mpeg_header_data *ret_header,
                                                int *eof);
#endif

/*
 * MPEG audio file time indexing routines header
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
 * SccsId[] = "$Id: mpegindx.h,v 1.6 2004/07/19 14:44:04 number6 Exp $"
 */

#ifndef _MPEGINDX_H
#define _MPEGINDX_H

#include <stdio.h>


int mp3edit_skip_xing_header(FILE *fp);
int mp3edit_create_indexfile(void *ifctx);
int mp3edit_isvalid_index(FILE *mpegfp, FILE *indxfp);

long mp3edit_indexfile_sec(void *ifctx);
long mp3edit_indexfile_msec(void *ifctx);
long mp3edit_indexfile_frames(void *ifctx);
long mp3edit_indexfile_offset(void *ifctx);
int  mp3edit_indexfile_get_stats(void *ifctx);

FILE *mpgedit_indexfile_init(FILE *fp,
                             char *mpeg_filename,
                             char **rindx_filename);

_DSOEXPORT long _CDECL mp3edit_get_size_from_index(FILE *indxfp, 
                                                   long *sec, long *usec);
_DSOEXPORT char *_CDECL mpgedit_index_build_filename(char *path);
_DSOEXPORT char *_CDECL mpgedit_index_build_filename_2(char *path, char *ext);

_DSOEXPORT char *_CDECL mpgedit_pathname_parse(
                            const char *path,
                            char **ret_filebase, char **ret_ext);

_DSOEXPORT int _CDECL validate_outfile(char *filename,
                                       int append, char **errstr);

_DSOEXPORT FILE * _CDECL mpgedit_indexfile_open(char *name, char *mode);

_DSOEXPORT void   _CDECL mpgedit_indexfile_close(FILE *fp);

#endif

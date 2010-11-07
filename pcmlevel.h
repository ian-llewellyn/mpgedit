/*
 * PCM digest file API
 *
 * Copyright (C) 2003 Adam Bernstein
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

#ifndef _PCMLEVEL_H
#define _PCMLEVEL_H

#include "portability.h"
#include "pcmlevel.h"
#include <stdio.h>

typedef struct _mpgedit_pcmfile_t mpgedit_pcmfile_t;

typedef struct _mpgedit_pcmlevel_index_t mpgedit_pcmlevel_index_t;

_DSOEXPORT mpgedit_pcmfile_t * 
                _CDECL mpgedit_pcmlevel_open(char *file, char *mode);

_DSOEXPORT void _CDECL mpgedit_pcmlevel_close(mpgedit_pcmfile_t *fp);

_DSOEXPORT int  _CDECL mpgedit_pcmlevel_seek(mpgedit_pcmfile_t *fp, int offset);

_DSOEXPORT long _CDECL mpgedit_pcmlevel_tell(mpgedit_pcmfile_t *ctx);

_DSOEXPORT void _CDECL mpgedit_pcmlevel_write_header(
                           mpgedit_pcmfile_t *ctx, int ver, int pcmbits,
                           int secbits, int msecbits);
_DSOEXPORT void _CDECL mpgedit_pcmlevel_write_entry(
                           mpgedit_pcmfile_t *ctx,
                           int pcmlevel,
                           long sec, long msec);
_DSOEXPORT void _CDECL mpgedit_pcmlevel_write_average(
                           mpgedit_pcmfile_t *ctx,
                           int avg, int max, int min);

_DSOEXPORT int  _CDECL mpgedit_pcmlevel_read_entry(
                           mpgedit_pcmfile_t *ctx,
                           int *pcmmax,
                           long *sec, long *msec);
_DSOEXPORT int  _CDECL mpgedit_pcmlevel_read_header(
                           mpgedit_pcmfile_t *ctx,
                           int *ver, int *pcmbits,
                           int *secbits, int *msecbits);
_DSOEXPORT int  _CDECL mpgedit_pcmlevel_read_average(
                           mpgedit_pcmfile_t *ctx,
                           int *avg,
                           int *max, int *min);

_DSOEXPORT long _CDECL mpgedit_pcmlevel_size(
                           mpgedit_pcmfile_t *ctx);

_DSOEXPORT long _CDECL mpgedit_pcmlevel_index_get_offset(
                           mpgedit_pcmfile_t *pcmh,
                           mpgedit_pcmlevel_index_t  *index,
                           long               sec);

_DSOEXPORT mpgedit_pcmlevel_index_t *
                _CDECL mpgedit_pcmlevel_generate_index(
                           mpgedit_pcmfile_t *pcmh);

_DSOEXPORT void _CDECL mpgedit_pcmlevel_index_free(
                           mpgedit_pcmlevel_index_t *index);

#endif

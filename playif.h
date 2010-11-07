/*
 * mpgedit file playback API header
 *
 * Copyright (C) 2001-2006 Adam Bernstein. All Rights Reserved.
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

#ifndef _PLAYIF_H
#define _PLAYIF_H 1

#include "portability.h"

#if  1 /* adam */
/*
 * Make all void * in these definitions this type
 */
typedef struct _mpgedit_play_t mpgedit_play_t;
#endif


_DSOEXPORT void * _CDECL mpgedit_play_init(char *name, unsigned long flags);
_DSOEXPORT int    _CDECL mpgedit_play_frame(void *_ctx);
_DSOEXPORT int    _CDECL mpgedit_play_seek_time(void *_ctx, long, long);
_DSOEXPORT void   _CDECL mpgedit_play_close(void *_ctx);
_DSOEXPORT long   _CDECL mpgedit_play_total_size(void *_ctx);
_DSOEXPORT long   _CDECL mpgedit_play_total_sec(void *_ctx);
_DSOEXPORT long   _CDECL mpgedit_play_total_msec(void *_ctx);
_DSOEXPORT long   _CDECL mpgedit_play_current_sec(void *_ctx);
_DSOEXPORT long   _CDECL mpgedit_play_current_msec(void *_ctx);

_DSOEXPORT void   _CDECL mpgedit_play_set_status_callback(
                                      void *_ctx,
                                      int (*status)(void *, long, long),
                                      void *data);

_DSOEXPORT void   _CDECL mpgedit_play_decode_set_status_callback(
                                      void *_ctx,
                                      int (*status)(void *, 
                                                    unsigned char *, long, 
                                                    long, long),
                                      void *data);

_DSOEXPORT int    _CDECL mpgedit_play_skip_frame(void *_ctx);


_DSOEXPORT int    _CDECL mpgedit_play_decode_frame(void *_ctx,
                                                   unsigned char **pcm_buf,
                                                   int *pcm_buf_len,
                                                   int *bsbytes);

_DSOEXPORT void   _CDECL mpgedit_play_decode_frame_header_info(
                             void *_ctx,
                             int *mpeg_ver,
                             int *mpeg_layer,
                             int *bit_rate,
                             int *sample_rate,
                             int *samples_per_frame,
                             int *frame_size,
                             int *channel_mode,
                             int *joint_ext_mode);
_DSOEXPORT int _CDECL mpgedit_play_reset_audio(
                          void *_ctx);

_DSOEXPORT void _CDECL mpgedit_play_volume_init(
                           void *_ctx,
                           int lvol,
                           int rvol);

_DSOEXPORT int _CDECL mpgedit_play_volume_get(
                          void *_ctx,
                          int *lvol,
                          int *rvol);

_DSOEXPORT int _CDECL mpgedit_play_volume_set(
                          void *_ctx,
                          int lvol,
                          int rvol);


#endif

/*
 * mpgedit statistics manipulation functions header file
 *
 * Copyright (C) 2001-2003 Adam Bernstein
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

#ifndef _MPEGSTAT_H
#define _MPEGSTAT_H

#include "portability.h"
#include "header.h"
#include "mp3time.h"

typedef struct _mpeg_file_stats
{
    char *filename;
    int min_bitrate;
    int max_bitrate;
    int avg_bitrate;
    long file_size;
    long num_frames;
    long bitrate_frame_cnt[16];
    int mpeg_version_index;
    int mpeg_layer;
    int inited;
} mpeg_file_stats;

int mpeg_file_stats_is_vbr(mpeg_file_stats *stats);

void mpeg_file_stats_init(mpeg_file_stats *s,
                          char *filename,
                          int offset,
                          int frames);

void mpeg_file_stats_gather(mpeg_file_stats *stats,
                            mpeg_header_data *header);

_DSOEXPORT void _CDECL mpeg_file_stats2str(mpeg_file_stats *stats,
                                           mpeg_time *time, char *str);

_DSOEXPORT void _CDECL mpeg_file_stats_join(mpeg_file_stats *global_stats,
                                            mpeg_file_stats *stat_entry);

#endif

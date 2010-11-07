/*
 * MPEG audio frame header parse and decode functions header
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
 * SccsId[] = "$Id: header.h,v 1.10 2005/09/08 06:18:55 number6 Exp $"
 */

#ifndef _HEADER_H
#define _HEADER_H
#include <stdio.h>
#include "portability.h"
#include "md5lib/md5.h"

#define MPGEDIT_ALLOW_MPEG1L1 1
typedef struct _mpeg_header_data {
    int mpeg_version;
    int mpeg_version_index;
    int mpeg_layer;
    int mpeg_layer_index;
    unsigned char protection;
    int br_index;
    int sr_index;

    int bit_rate;     /* decoded br_index value */
    int sample_rate;  /* decoded sr_index value */
    int samples_per_frame;
    int frame_size;   /* Number of bytes present in frame */
    long position;

    unsigned char pad;
    unsigned char private;
    int channel_mode;
    int joint_ext_mode;
    unsigned char copyright;
    unsigned char original;
    int emphasis;
    int main_data_begin;

    int do_md5sum;
    md5_byte_t md5sum[16];
} mpeg_header_data;

int decode_mpeg_header(unsigned char *data,
                       mpeg_header_data *header,
                       unsigned char v1l1_flag);

_DSOEXPORT int _CDECL mpgedit_header_decode(unsigned char *data,
                                            mpeg_header_data *header,
                                            unsigned char v1l1_flag);

_DSOEXPORT void _CDECL mpeg_header_values2str(mpeg_header_data *h, char *cp);

_DSOEXPORT void _CDECL mpeg_header_values2str_3(mpeg_header_data *h,
                                                long filepos, char *cp);
int find_mpeg_header_buf(unsigned char *buf,
                         int len,
                         mpeg_header_data *header,
                         unsigned char v1l1_flag);


int get_bitrate_from_index(int version, int layer, int index);

#endif

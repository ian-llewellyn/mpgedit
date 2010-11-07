/*
 * mpgedit file playback API private types header
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

#include "editif.h"
#include "md5lib/md5.h"
#ifdef _EDITIF_C
struct _mpgedit_t
#else
struct _playctx
#endif
{

  /* typedef struct _editfile_ctx { */
    mpeg_file_iobuf  mpegiobuf;
    mpeg_time        stime;
    mpeg_time        etime;
    int              tnow;
    int              tlast;
    /* 
     * adam/TBD: file_position and frame_cnt appear to be redundant with
     * num_frames and file_size in mpegstat.h.  These two variables
     * should be refactored out.
     */
    long             file_position;
    int              frame_cnt;

    int              inited;
    int              loop;
  /* } editfile_ctx;                */

/* 
 * How are "header" and "stats_frame_header" used differently?
 * Can one of these variables be eliminated?
 */
    mpeg_header_data header;
    mpeg_header_data stats_frame_header;
/* 
 * How are "stime" and "mpegtval" used differently?
 * Can one of these variables be eliminated?
 */
    mpeg_time        mpegtval;
    void             *mpctx;
    char             *name;
    char             *indx_name;
    FILE             *mpegfp;
    FILE             *indxfp;
    FILE             *playfp;
    long             fsize;
    long             sec;
    long             usec;
    long             cursec;
    long             curusec;
    /* play callback and data */
    int              (*fp_status)(void *, long, long);
    void             *dp_status;
    /* Decode callback and data */
    int              (*fp_decode_status)(void *, unsigned char *, long,
                                         long, long);
    void             *dp_decode_status;
    /* Index callback and data */
    int              (*fp_index)(void *, long, long);
    void             *dp_index;

/* mpgedit_edit_files state variables */
    enum mpgedit_edit_files_states state;
    int              editsindx;
    FILE             *editfp;
    int              has_xing;
    mpeg_file_stats  stats;
    mpeg_file_stats  *g_stats;
    XHEADDATA        xingh;

    /* Command line arguments from mpgedit_edit_files () */
    editspec_t       *edarray;
    int              edlen;
    char             *outfile;
    unsigned int     flags;
    unsigned int     outfile_exists;

    /* 
     * seeked is set when mpgedit_play_seek_time() has been called.  Tells
     * mpgedit_play_frame() it needs to initially perform a
     * mpgdecoder_play_skip_frame() prior to mpgdecoder_play_frame().
     */
    int seeked;
    mpeg_file_iobuf  prev_mpegiobuf;
    int prev_framesize;

    /*
     * MD5 frame checksum variables
     */
    int        do_md5sum;

    /*
     * Left/right channel volume levels
     */
    int vol_left;
    int vol_right;
};

#ifndef _EDITIF_C
typedef struct _playctx playctx;
#endif

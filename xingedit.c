/*
 * XING header editing abstraction
 *
 * Copyright (C) 2009 Adam Bernstein
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xing_header.h"
#include "xingedit.h"
#include "mpegstat.h"
#include "mpegfio.h"
#include "header.h"


/*
 * Internal function called by public API to do all of the
 * editing work. The open file must be opened for both read
 * and write. The value of frames/bytes are either set, or
 * incremented, according to action. Set either frames or
 * bytes to -1 to ignore that attribute.
 */
static int _mpgedit_xing_modify(
               FILE *fp,
               long frames,
               long bytes,
               mpeg_file_stats *stats,
               MPGEDIT_XING_MODIFY_ACTION_en action)
{
    mpeg_header_data mpeg_header;
    XHEADDATA        X;
    int              rsts = 0;
    int              header_size;
    int              edit_frames;
    int              edit_bytes;
    int              is_vbr = 0;

    memset(&mpeg_header, sizeof(mpeg_header), 0);
    memset(&X, 0, sizeof(X));

    if (!fp) {
        return 1;
    }
    rsts = fseek(fp, 0, SEEK_SET);
    if (rsts == -1) {
        return 1;
    }
    
    header_size = mpegfio_has_xing_header(fp, &mpeg_header, &X, 0);
    if (header_size == 0) {
        return 1;
    }
    else {
        is_vbr = 1;
    }

    if (stats) {
        is_vbr = mpeg_file_stats_is_vbr(stats);
    }
    switch(action) {
      case MPGEDIT_XING_MODIFY_SET:
      default:
        edit_frames = (frames == -1) ? X.frames : frames;
        edit_bytes  = (bytes == -1)  ? X.bytes  : bytes;
        break;
      case MPGEDIT_XING_MODIFY_INCR:
        edit_frames = X.frames + (frames == -1 ? 0 : frames);
        edit_bytes  = X.bytes  + (bytes == -1 ? 0  :  bytes);
        break;
    }

    edit_xing_header(edit_frames, edit_bytes, 
                     X.xingbuf, X.xingbuflen, X.h_id, X.h_mode,
                     is_vbr);

    /* Write modified Xing header back to MP3 file */
    if (xingheader_init(X.xingbuf, header_size, &X)) {
        rsts = fseek(fp, 0, SEEK_SET);
        if (rsts != -1) {
            rsts = (fwrite(X.xingbuf, header_size, 1, fp) == 1) ? 0 : 1;
        }
        else {
            rsts = 1;
        }
    }
    return rsts;
}


int mpgedit_xing_modify_fp(FILE *fp, long frames, long bytes, MPGEDIT_XING_MODIFY_ACTION_en action)
{
    return _mpgedit_xing_modify(fp, frames, bytes, NULL, action);
}


int mpgedit_xing_modify_file(char *name, 
                             long frames, 
                             long bytes, 
                             MPGEDIT_XING_MODIFY_ACTION_en action)
{
    FILE *fp;
    int rsts;

    
    fp = fopen(name, "rb+");
    if (!fp) {
        return 1;
    }
    rsts = _mpgedit_xing_modify(fp, frames, bytes, NULL, action);
    fclose(fp);
    return(rsts);
}


int mpgedit_xing_stats_modify_fp(
        FILE *fp,
        mpeg_file_stats *stats,
        MPGEDIT_XING_MODIFY_ACTION_en action)
{
    return _mpgedit_xing_modify(fp, stats->num_frames, stats->file_size, stats, action);
}


int mpgedit_xing_stats_modify_file(
        char *name, 
        mpeg_file_stats *stats,
        MPGEDIT_XING_MODIFY_ACTION_en action)
{
    FILE *fp;
    int  rsts;

    fp = fopen(name, "rb+");
    if (!fp) {
        return 1;
    }
    rsts = mpgedit_xing_stats_modify_fp(fp, stats, action);
    fclose(fp);

    return rsts;
}

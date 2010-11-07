/*
 * Microsecond resolution time accounting using fixed point math.
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

#ifndef lint
static char SccsId[] = "$Id: mp3time.c,v 1.16.2.4 2009/01/21 16:26:24 number6 Exp $";
#endif

#include "portability.h"
#include "mp3time.h"
#include "header.h"
#include "mpegfio.h"


void mpeg_time_clear(mpeg_time *tval)
{
    memset(tval, 0, sizeof(*tval));
}


void mpeg_time_frame_increment(mpeg_time *tval, mpeg_header_data *header)
{
    tval->usec += header->samples_per_frame * 1000000 / header->sample_rate;
    if (tval->usec >= 1000000) {
        tval->units += 1;
        tval->usec -= 1000000;
    }
}


long mpeg_time_2seconds(mpeg_time *tval)
{
    return tval->units + (tval->usec / 1000000);
}


void mpeg_time_gettime(mpeg_time *tval, long *sec, long *usec)
{
    *sec = tval->units;
    *usec = tval->usec;
}



void mpeg_time_init(mpeg_time *tval, int sec, int usec)
{
    tval->units = sec;
    tval->usec = usec;
}


long mpeg_time_getsec(mpeg_time *tval)
{
    return tval->units;
}

long mpeg_time_getusec(mpeg_time *tval)
{
    return tval->usec;
}


void mpeg_time_string2time(char *s, mpeg_time *tval)
{
    char *sp;
    long sec  = 0;
    long usec = 0;

    if (!s) {
        return;
    }
    sp = s = strdup(s);
    if (!s) {
        return;
    }

    sec = strtol(sp, &sp, 10);
    if (sp && *sp == ':') {
        sec *= 60;
        sec += strtol(sp+1, &sp, 10);
    }
    if (sp && (*sp == '.' || *sp == ':')) {
        usec = strtol(sp+1, &sp, 10) * 1000;
        if (usec > 999000) {
            usec = 999000;
        }
    }
    mpeg_time_init(tval, sec, usec);
    free(s);
}


long mpeg_time_get_seek_offset(FILE *indxfp,
                               mpeg_time *stime,
                               mpeg_time *seektime)
{
    int32_t          seek_file_pos = 0;
    int32_t          usec_residual = 0;
    long             stime_sec  = 0;
    long             stime_usec = 0;
    int              sts;
    int32_t          version;
    long             seek_offset;
    int              indx_eof_flag = 0;

    if (!indxfp || !stime) {
        return 1;
    }

    /*
     * When a time specification is provided, move to initial position in
     * stream.  Seek to second offset, then search for specified subframe
     * when present.
     */
    if (fseek(indxfp, 0, SEEK_SET) == -1) {
        return -1;
    }
    sts = fread(&version, sizeof(version), 1, indxfp);
    if (sts != 1) {
        return -1;
    }
    if (version) {
        /* Found version 2 index file */
        sts = fread(&version, sizeof(version), 1, indxfp);
        if (sts != 1) {
            return -1;
        }
        version = ExtractI4((unsigned char *) &version);
    }

    if (stime) {
        mpeg_time_gettime(stime, &stime_sec, &stime_usec);
    }

    if (version > 0) {
        /*
         * Add 1 to seek second because first entry is 
         * version information
         */
        seek_offset = (1+stime_sec) * sizeof(int32_t) * 2;
        if (version == 3) {
            seek_offset += 100;
        }
    }
    else {
        seek_offset = stime_sec * sizeof(int32_t);
    }

    if (fseek(indxfp, seek_offset, SEEK_SET) == -1) {
        return -1;
    }
    sts = fread(&seek_file_pos, sizeof(seek_file_pos), 1, indxfp);
    if (sts != 1) {
        return -1;
    }
    if (version > 0) {
        seek_file_pos = ExtractI4((unsigned char *) &seek_file_pos);
        sts = fread(&usec_residual, sizeof(usec_residual), 1, indxfp);
        if (sts != 1) {
            return -1;
        }
        usec_residual = ExtractI4((unsigned char *) &usec_residual);
        indx_eof_flag = usec_residual & (1U<<31);
    }

#if defined(_MPGEDIT_DEBUG)
    printf("Seeking file offset: 0x%lx | %lu; usec: 0x%lx : %lu\n",
           seek_file_pos, seek_file_pos, usec_residual, usec_residual);
#endif
    /*
     * The last index file entry is the EOF entry, containing
     * the total MP3 file size, and the last time offset.  This is not
     * a valid entry for time seeking, so it is ignored here.
     */
    if (indx_eof_flag) {
        return -1;
    }
    mpeg_time_init(seektime, stime_sec, usec_residual);
    return seek_file_pos;
}


long mpeg_time_seek_starttime(FILE      *fp,
                              FILE      *indxfp,
                              mpeg_time *stime,
                              mpeg_time *mpegtval,
                              int       has_xing)
{
    long             seek_file_pos = 0;
    long             usec_residual = 0;
    int              found = 0;
    mpeg_file_iobuf  mpegiobuf;
    mpeg_header_data header;
    long             stime_sec  = 0;
    long             stime_usec = 0;
    long             sec        = 0;
    long             usec       = 0;
    int              eof = 0;


    seek_file_pos = mpeg_time_get_seek_offset(indxfp, stime, mpegtval);
    if (seek_file_pos == -1) {
        return -1;
    }

#if defined(_MPGEDIT_DEBUG)
    printf("Seeking file offset: 0x%lx | %lu; usec: 0x%lx : %lu\n",
           seek_file_pos, seek_file_pos, usec_residual, usec_residual);
#endif

    if (fseek(fp, seek_file_pos, SEEK_SET) == -1) {
        return -1;
    }
    seek_file_pos = ftell(fp);
    mpeg_time_gettime(mpegtval, &stime_sec, &usec_residual);
    mpeg_time_gettime(stime,    &stime_sec, &stime_usec);

    /*
     * When seeking to the beginning of the file, always skip over XING 
     * header, when present.  The XING header has already been copied to 
     * the output file, so it must never be included here.
     */
    if (seek_file_pos == 0 && has_xing) {
        fseek(fp, has_xing, SEEK_SET);
    }

    /*
     * Seek to a sub-second frame offset. Read frames until the
     * the microsecond time total just exceeds the desired time offset.
     * Each frame is approximately 26ms long.
     */
    if (stime_usec > 0) {
        memset(&header, 0, sizeof(header));
        mpeg_file_iobuf_clear(&mpegiobuf);

        mpeg_time_gettime(mpegtval, &sec, &usec);
        while (sec <= stime_sec && usec < stime_usec && found != -1 && !eof) {
            found = read_next_mpeg_frame(fp, &mpegiobuf, &header, &eof);
            if (found) {
                mpeg_time_frame_increment(mpegtval, &header);
                mpeg_file_iobuf_setstart(&mpegiobuf,
                                         mpeg_file_iobuf_getstart(&mpegiobuf) +
                                             header.frame_size);
            }
            mpeg_time_gettime(mpegtval, &sec, &usec);
        }
        if (eof && sec <= stime_sec && usec < stime_usec) {
            return -1;
        }

        /*
         * Seek back to the portion of the stream which has not
         * been skipped over.  Remember mpegiobuf buffers data from the
         * input file in 4K blocks, so it is certain there will be a
         * residual that must be kept.
         */
        if (found != -1) {
            fseek(fp, mpegiobuf.start - mpegiobuf.len, SEEK_CUR);
            seek_file_pos = ftell(fp);
        }
    }
    return seek_file_pos;
}


mpeg_time mpeg_time_compute_delta(long ssec, long susec, long esec, long eusec)
{
    long dsec;
    long dusec;
    mpeg_time dsecs;

    dsec = esec - ssec;
    dusec = eusec - susec;
    if (dusec < 0 && dsec > 0) {
        dsec--;
        dusec += 1000000;
    }
    dsecs.units = dsec;
    dsecs.usec  = dusec;
    return dsecs;
}

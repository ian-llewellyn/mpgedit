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

#define _MPGEDIT_PCMLEVEL_TIMEINDEX_OFFSET (4 + 6)

/* header(32) | average(48) | timeindx_position (32) | 
 *     PCM entries ...| timeindex(N) 
 */

/*
 * Header format:     version(8)  | pcmbits(8)  | secbits(8) | msecbits(8)
 * Average format:    average(16) | maxpcm(16)  | minpcm(16)
 * PCM Entry format:  pcmval(16)  | sec(22)     | msec(10)
 * Time index format: foffset(32)
 *   One entry for every 10 seconds of data.
 */

#include <stdio.h>
#include "pcmlevel.h"
#include "xing_header.h"

struct _mpgedit_pcmfile_t
{
    FILE  *fp;
};


struct _mpgedit_pcmlevel_index_entry_t
{
    long offset;
};


struct _mpgedit_pcmlevel_index_t {
    struct _mpgedit_pcmlevel_index_entry_t *entry;
    long                                   entries;
};


void mpgedit_pcmlevel_write_header(mpgedit_pcmfile_t *ctx,
                                   int ver, int pcmbits,
                                   int secbits, int msecbits)
{
    unsigned char buf[1];
    int sts;

    /*
     * Header format: version | pcmbits | secbits | msecbits
     */

    /* version */
    buf[0] = ver;
    sts = fwrite(buf, sizeof(buf), 1, ctx->fp);
    if (sts != 1) {
        return;
    }

    /* pcmbits */
    buf[0] = pcmbits;
    sts = fwrite(buf, sizeof(buf), 1, ctx->fp);
    if (sts != 1) {
        return;
    }

    /* secbits */
    buf[0] = secbits;
    sts = fwrite(buf, sizeof(buf), 1, ctx->fp);
    if (sts != 1) {
        return;
    }

    /* msecbits */
    buf[0] = msecbits;
    sts = fwrite(buf, sizeof(buf), 1, ctx->fp);
    if (sts != 1) {
        return;
    }
}


mpgedit_pcmfile_t *mpgedit_pcmlevel_open(char *file, char *mode)
{
    mpgedit_pcmfile_t *ctx;
    FILE              *fp;

    fp = fopen(file, mode);
    if (!fp) {
        return NULL;
    }

    ctx = (mpgedit_pcmfile_t *) calloc(1, sizeof(mpgedit_pcmfile_t));
    if (ctx) {
        ctx->fp = fp;
    }
    else {
        fclose(fp);
    }
    return ctx;
}


void mpgedit_pcmlevel_close(mpgedit_pcmfile_t *ctx)
{
    if (ctx && ctx->fp) {
        fclose(ctx->fp);
    }
}


int mpgedit_pcmlevel_seek(mpgedit_pcmfile_t *ctx, int offset)
{
    int  rsts = -1;
    long file_size;

    if (ctx && ctx->fp) {
        fseek(ctx->fp, 0, SEEK_END);
        file_size = ftell(ctx->fp);
        if (offset >= file_size) {
            rsts = -1;
        }
        else {
            rsts = fseek(ctx->fp, offset, SEEK_SET);
        }
    }
    return rsts;
}



long mpgedit_pcmlevel_tell(mpgedit_pcmfile_t *ctx)
{
    long rsts = -1;

    if (ctx && ctx->fp) {
        rsts = ftell(ctx->fp);
    }
    return rsts;
}



long mpgedit_pcmlevel_size(mpgedit_pcmfile_t *ctx)
{
    long rsts = -1;
    long save;

    if (ctx && ctx->fp) {
        save = ftell(ctx->fp);
        if (save != -1) {
            fseek(ctx->fp, 0, SEEK_END);
            rsts = ftell(ctx->fp);
            fseek(ctx->fp, save, SEEK_SET);
        }
    }
    return rsts;
    
}


void mpgedit_pcmlevel_write_average(mpgedit_pcmfile_t *ctx,
                                    int avg, int max, int min)
{
    unsigned char buf[6];
    int sts;

    InsertI2(avg, buf);
    InsertI2(max, buf+2);
    InsertI2(min, buf+4);
    sts = fwrite(buf, sizeof(buf), 1, ctx->fp);
}


#if 0
int mpgedit_pcmlevel_write_timeindex_offset(
        mpgedit_pcmfile_t *ctx, long offset, int seek)
{
    long rsts = -1;
    unsigned char buf[4]

    if (seek) {
        rsts =  fseek(ctx->fp, _MPGEDIT_PCMLEVEL_TIMEINDEX_OFFSET, SEEK_SET);
        if (rsts == -1) {
            goto clean_exit;
        }
    }
    InsertI4(offset, buf);
    rsts = fwrite(buf, sizeof(buf), 1, ctx->fp);
    if (rsts != 1) {
        rsts = -1;
        goto clean_exit;
    }

clean_exit:
    return rsts;
}
#endif


void mpgedit_pcmlevel_write_entry(mpgedit_pcmfile_t *ctx,
                                  int pcmlevel, long sec, long msec)
{
    unsigned char buf[6];
    int sts;

    if (!ctx->fp) {
        return;
    }

    InsertI2(pcmlevel, buf);
    InsertB2210(buf+2, sec, msec);
    sts = fwrite(buf, sizeof(buf), 1, ctx->fp);
}



int mpgedit_pcmlevel_read_header(mpgedit_pcmfile_t *ctx,
                                 int *ver, int *pcmbits,
                                 int *secbits, int *msecbits)
{
    unsigned char buf[1];
    int sts = 1;

    if (fread(buf, sizeof(buf), 1, ctx->fp)) {
        *ver = (int) buf[0];
    }
    else {
        sts = 0;
    }
    if (fread(buf, sizeof(buf), 1, ctx->fp)) {
        *pcmbits = (int) buf[0];
    }
    else {
        sts = 0;
    }
    if (fread(buf, sizeof(buf), 1, ctx->fp)) {
        *secbits = (int) buf[0];
    }
    else {
        sts = 0;
    }
    if (fread(buf, sizeof(buf), 1, ctx->fp)) {
        *msecbits = (int) buf[0];
    }
    else {
        sts = 0;
    }
    return sts;
}


int mpgedit_pcmlevel_read_entry(mpgedit_pcmfile_t *ctx,
                                int *pcmmax, long *rsec, long *rmsec)
{
    unsigned char buf[6];
    int           sts = 1;
    int           sec;
    int           msec;

    if (!ctx) {
        return 0;
    }
    if (!pcmmax && !rsec && !rmsec) {
        return 0;
    }

    if (fread(buf, sizeof(buf), 1, ctx->fp)) {
        if (pcmmax) {
            *pcmmax = ExtractI2(buf);
        }
        if (rsec || rmsec) {
            ExtractB2210(buf+2, &sec, &msec);
            if (rsec) {
                *rsec  = sec;
            }
            if (rmsec) {
                *rmsec = msec;
            }
        }
    }
    else {
        sts = 0;
    }
    return sts;
}



int mpgedit_pcmlevel_read_average(mpgedit_pcmfile_t *ctx,
                                  int *avg, int *max, int *min)
{
    unsigned char buf[6];
    int sts = 1;

    if (fread(buf, sizeof(buf), 1, ctx->fp)) {
        if (avg) {
            *avg = ExtractI2(buf);
        }
        if (max) {
            *max = ExtractI2(buf+2);
        }
        if (min) {
            *min = ExtractI2(buf+4);
        }
    }
    else {
        sts = 0;
    }
    return sts;
}


#if 0
long mpgedit_pcmlevel_read_timeindex_offset(mpgedit_pcmfile_t *ctx, int seek)
{
    long rsts = -1;
    unsigned char buf[4]

    if (seek) {
        rsts =  fseek(ctx->fp, _MPGEDIT_PCMLEVEL_TIMEINDEX_OFFSET, SEEK_SET);
        if (rsts == -1) {
            goto clean_exit;
        }
    }
    rsts = fread(buf, sizeof(buf), 1, ctx->fp);
    if (rsts != 1) {
        rsts = -1;
        goto clean_exit;
    }
    rsts = ExtractI4(buf);

clean_exit:
    return rsts;
}
#endif


#if 0
/*
 * Return index file position based on time offset base then indexed
 * by a entry count
 */
long mpgedit_pcmlevel_index_get_index(mpgedit_pcmfile_t         *pcmh,
                                      mpgedit_pcmlevel_index_t  *index,
                                      long                      sec,
                                      long                      indexval)
{
    long fpos;
    fpos = mpgedit_pcmlevel_index_get_offset(pcmh, index, sec);
    fpos += indexval * 6;
}
#endif



long mpgedit_pcmlevel_index_get_offset(mpgedit_pcmfile_t         *pcmh,
                                       mpgedit_pcmlevel_index_t  *index,
                                       long                      sec)
{
    int  seek_indx;
    int  seek_residual;
    long seek_offset;
    int  go;
    int  pcmlevel;
    long pcmsec, pcmmsec;


    if (!pcmh || !index || sec < 0) {
        return -1;
    }

    seek_indx     = sec / 10;
    seek_residual = sec % 10;
    if (seek_indx < 0) {
        seek_indx = 0;
    }
    else if (seek_indx >= index->entries) {
        seek_indx = index->entries;
    }
    seek_offset = index->entry[seek_indx].offset;

    mpgedit_pcmlevel_seek(pcmh, seek_offset);

    if (seek_residual) {
        go = mpgedit_pcmlevel_read_entry(pcmh, &pcmlevel, &pcmsec, &pcmmsec);
        while (go && pcmsec < sec) {
            go = mpgedit_pcmlevel_read_entry(pcmh, &pcmlevel,
                                             &pcmsec, &pcmmsec);
        }
        seek_offset = mpgedit_pcmlevel_tell(pcmh);
        if (!go) {
            seek_offset = -1;
        }
    }
    return seek_offset;
}


mpgedit_pcmlevel_index_t *mpgedit_pcmlevel_generate_index(
                             mpgedit_pcmfile_t *pcmh)
{
    int                      ver=0, pcmbits=0, secbits=0, msecbits=0;
    int                      pcmavg, pcmmin, pcmmax;
    int                      level;
    long                     sec;
    long                     msec;
    long                     lastsec = 0;
    long                     fpos_save;
    int                      go;
    mpgedit_pcmlevel_index_t *index   = NULL;
    int                      i        = 0;
    int                      err      = 0;
    int                      num_entries;

    if (!pcmh) {
        return NULL;
    }
    fpos_save = mpgedit_pcmlevel_tell(pcmh);
    
    /*
     * Assume between on average 3.5 samples per second.  For an index
     * resolution of 10, that is 35 samples per entry.  Each sample
     * is then 6 bytes long.
     */
    num_entries = mpgedit_pcmlevel_size(pcmh) / (35 * 6);
    index = (mpgedit_pcmlevel_index_t *)
                calloc(1, sizeof(mpgedit_pcmlevel_index_t));
    if (!index) {
        err = 1;
        goto clean_exit;
    }
    index->entry = (struct _mpgedit_pcmlevel_index_entry_t *)
                       calloc(num_entries, 
                              sizeof(struct _mpgedit_pcmlevel_index_entry_t));
    if (!index->entry) {
        err = 1;
        goto clean_exit;
    }

    mpgedit_pcmlevel_seek(pcmh, 0);
    mpgedit_pcmlevel_read_header(pcmh, &ver, &pcmbits, &secbits, &msecbits);
    if (ver!=1 || pcmbits!=16 || secbits!=22 || msecbits!=10) {
        err = 1;
        goto clean_exit;
    }
    mpgedit_pcmlevel_read_average(pcmh, &pcmavg, &pcmmin, &pcmmax);
    
    index->entry[i++].offset = mpgedit_pcmlevel_tell(pcmh);
    go = mpgedit_pcmlevel_read_entry(pcmh, &level, &sec, &msec);
    while (go) {
        if (sec >= lastsec + 10) {
            if (i < num_entries) {
                index->entry[i++].offset = mpgedit_pcmlevel_tell(pcmh);
            }
            else {
                go = 0;
            }
            lastsec = sec;
        }
        if (go) {
            go = mpgedit_pcmlevel_read_entry(pcmh, &level, &sec, &msec);
        }
    }
    index->entries = i;

clean_exit:
    mpgedit_pcmlevel_seek(pcmh, fpos_save);
    if (err) {
        if (index) {
            if (index->entry) {
                free(index->entry);
            }
            free(index), index = NULL;
        }
    }
    return index;
}


void mpgedit_pcmlevel_index_free(mpgedit_pcmlevel_index_t *index)
{
    if (index) {
        if (index->entry) {
            free(index->entry);
        }
        free(index);
    }
}

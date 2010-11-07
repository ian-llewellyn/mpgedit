/*
 * mpgedit file playback API
 *
 * Copyright (C) 2001-2006 Adam Bernstein
 * All rights reserved.
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


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/stat.h>
#include "portability.h"
#include "header.h"
#include "mp3time.h"
#include "mpegindx.h"
#include "xing_header.h"
#include "mpegfio.h"
#include "mp3_header.h"
#include "mpegcurses.h"
#include "decoder.h"
#include "plugin.h"
#include "version.h"
#include "playif.h"
#include "p_playif.h"
#include "editif.h"

static plugin_fcn_dsc *fp_mpgdecoder;
static int            fp_mpgdecoder_refcnt;

static int _mpgedit_play_skip_frame(void *_ctx);


#define MPGEDIT_PLAYCTX(a) ((playctx *) (a))
#define FRAME_MSEC      26

/*
 * Search paths for decoder plugin.  Add to this list as needed.
 */
static  char *plugin_paths[] = {
    "/usr/local/lib",  /* Default install directory */
    "/opt/local/lib",
    "/sw/lib",         /* Location plugins end up in Fink package */
    "/usr/lib",
    "/lib",
    NULL
};


#if 1
/*
 * I don't know if this is useful, or correct yet.
 * This is an attempt at enabling playback of multiple
 * files without closing the sound device.
 */
void *mpgedit_play_name_init(char *name, void *_ctx)
{
    struct stat sbuf;
    FILE *fp;
    long    rsts = 0;
    long    sec;
    long    usec;
    playctx          *ctx = MPGEDIT_PLAYCTX(_ctx);
    
    if (!ctx) {
        return NULL;
    }

    if (stat(name, &sbuf) == -1) {
        return NULL;
    }

    if (!S_ISREG(sbuf.st_mode)) {
        return NULL;
    }

    fp = fopen(name, "rb");
    if (!fp) {
        fprintf(stderr, "fopen: test1.mp3 failed %s\n", strerror(errno));
        return NULL;
    }
    
    ctx->name   = name;
    if (ctx->mpegfp) {
        fclose(ctx->mpegfp);
    }
    ctx->mpegfp = fp;
    mpeg_file_stats_init(&ctx->stats, name, 0,0);

    if (ctx->indxfp) {
        fclose(ctx->indxfp);
    }
    ctx->indxfp = mpgedit_indexfile_init(ctx->mpegfp, ctx->name, NULL);
    if (ctx->indxfp) {
        /*
         * Get file size from index file
         */
        rsts = mp3edit_get_size_from_index(ctx->indxfp, &sec, &usec);
        if (rsts != -1) {
            ctx->fsize = rsts;
            ctx->sec   = sec;
            ctx->usec  = usec;
        }
    }
    memset(&ctx->header, 0,  sizeof(ctx->header));
    memset(&ctx->xingh,  0,  sizeof(ctx->xingh));
    ctx->has_xing = mpegfio_has_xing_header(ctx->mpegfp,
                                            &ctx->header,
                                            &ctx->xingh, 0);

    mpeg_file_stats_init(&ctx->stats, name, ctx->has_xing, 0);

    return ctx;
}
#endif


/*
 * Obviously, all of these static functions need to be moved into mpegindx.c
 * Major refactoring here!!!
 */
void *mpgedit_play_init(char *name, unsigned long flags)
{
    char    *loaderrmsg;
    playctx *ctx = NULL;
    long    rsts = 0;
    long    sec;
    long    usec;
    FILE    *fp = NULL;
    void    *opened = NULL;
    struct stat sbuf;

    if (name) {
        if (stat(name, &sbuf) == -1) {
            return NULL;
        }
    
        if (!S_ISREG(sbuf.st_mode)) {
            return NULL;
        }
    
        fp = fopen(name, "rb");
        if (!fp) {
            fprintf(stderr, "fopen: test1.mp3 failed %s\n", strerror(errno));
            return NULL;
        }
    }

    ctx = calloc(1, sizeof(playctx));
    if (!ctx) {
        rsts = -1;
        goto clean_exit;
    }
    ctx->name   = name;
    ctx->mpegfp = fp;
    ctx->flags  = flags;
    mpeg_file_stats_init(&ctx->stats, name, 0,0);


    fp_mpgdecoder = mpgfio_alloc_decoder_plugin();
    if (!fp_mpgdecoder) {
        fprintf(stderr, "Failed allocating decoder plugin\n");
        rsts = -1;
        goto clean_exit;
    }
    fp_mpgdecoder_refcnt++;

    /*
     * Load MP3 file playback plugin.
     */
    loaderrmsg = mpegfio_load_decoder_plugin3("libmpgedit_decoder",
                                              fp_mpgdecoder, plugin_paths);
    if (loaderrmsg) {
        fprintf(stderr,
                "Error while loading plugin 'libmpgedit_decoder': %s\n",
                loaderrmsg);
        fprintf(stderr, "%s\n",
            "Warning: decoder.so plugin not found; default decoder used");
        fprintf(stderr, "%s\n",
            "         set LD_LIBRARY_PATH or install with ldconfig\n");
        free(loaderrmsg);
        rsts = -1;
        goto clean_exit;
    }
    ctx->mpctx = fp_mpgdecoder_alloc();
    if (!ctx->mpctx) {
        fprintf(stderr, "allocating mpgdecoder context failed\n");
        rsts = -1;
        goto clean_exit;
    }
    /*
     * playfp is not really a file pointer.  The function signature
     * was never changed to void * after this change was made.
     */
    opened = fp_mpgdecoder_open(ctx->mpctx, 44100, 0);
    if (!opened) {
        if (!(flags & MPGEDIT_FLAGS_NO_AUDIODEV)) {
            rsts = -1;
            goto clean_exit;
        }
    }

    fp_mpgdecoder_init(ctx->mpctx);
    ctx->fsize = ctx->sec = ctx->usec = -1;
    if (name) {
        ctx->indxfp = mpgedit_indexfile_init(ctx->mpegfp, ctx->name, NULL);
        if (ctx->indxfp) {
            /*
             * Get file size from index file
             */
            rsts = mp3edit_get_size_from_index(ctx->indxfp, &sec, &usec);
            if (rsts != -1) {
                ctx->fsize = rsts;
                ctx->sec   = sec;
                ctx->usec  = usec;
            }
        }
        memset(&ctx->header, 0,  sizeof(ctx->header));
        memset(&ctx->xingh,  0,  sizeof(ctx->xingh));
        ctx->has_xing = mpegfio_has_xing_header(ctx->mpegfp,
                                                &ctx->header,
                                                &ctx->xingh, 0);
    
        mpeg_file_stats_init(&ctx->stats, name, ctx->has_xing, 0);
    }

clean_exit:
    if (rsts == -1) {
        if (fp) {
            fclose(fp);
        }
        if (ctx) {
            if (ctx->mpctx) {
                fp_mpgdecoder_free(ctx->mpctx);
            }
            free(ctx);
            ctx = NULL;
        }
        /*
         * There may be another instance of a play context that will be using
         * this global pointer to the plugin function pointers.  Only free
         * this when the reference count goes to zero.
         */
        fp_mpgdecoder_refcnt--;
        if (fp_mpgdecoder_refcnt == 0 && fp_mpgdecoder) {
            free(fp_mpgdecoder);
        }
    }
#if 0 /* there are echo issues in doing this skip here */
    /*
     * adam/TBD:  However, there are also echo issues by NOT doing
     * the skip here.  This needs further research.
     */
    else if (name) {
        _mpgedit_play_skip_frame(ctx);
    }
#endif

    return ctx;
}


int mpgedit_play_seek_time(void *_ctx, long sec, long msec)
{
    playctx          *ctx = MPGEDIT_PLAYCTX(_ctx);
    long             offset;
    int              prev = 1;
    mpeg_time        residual_time;
    long             res_sec  = 0;
    long             res_msec = 0;
    extern long mpeg_time_read_seek_starttime(playctx *ctx, mpeg_time *etime);

    if (!ctx || (sec==0 && msec==0)) {
        return 0;
    }
    memset(&ctx->mpegtval, 0, sizeof(ctx->mpegtval));
    memset(&ctx->stime,    0, sizeof(ctx->stime));

    /*
     * Initially seek to the desired location to retrieve
     * the real time offset for the requested time.
     */
    ctx->stime.units = sec;
    ctx->stime.usec  = msec * 1000;
    if (ctx->flags & MPGEDIT_FLAGS_NO_INDEX) {
        prev = 0;
        offset = mpeg_time_read_seek_starttime(ctx, &residual_time);
        mpeg_file_iobuf_clear(&ctx->mpegiobuf);
    }
    else {
        offset = mpeg_time_seek_starttime(
                     ctx->mpegfp, 
                     ctx->indxfp, 
                     &ctx->stime, 
                     &residual_time,
                     ctx->has_xing);
    }
    if (offset != -1) {
        mpeg_time_gettime(&residual_time, &res_sec, &res_msec);
        res_msec /= 1000;
    }

    /* 
     * Skip backwards one frame, 26ms; use 28 to accomodate rounding
     * Save this previous frame for playback/decoding/editing.  The previous
     * frame is needed for backskip bits; this is a layer3 backskip thing.
     */
    res_msec = res_msec - (FRAME_MSEC + 2);
    if (res_msec < 0) {
        if ((sec-1) < 0) {
            ctx->stime.units = 0;
            ctx->stime.usec  = 0;
            prev = 0;
        }
        else {
            ctx->stime.units = sec - 1;
            ctx->stime.usec  = (res_msec+1000) * 1000;
        }
    }
    else {
        ctx->stime.units = sec;
        ctx->stime.usec  = res_msec * 1000;
    }

    ctx->seeked = 0;
    if (prev) {
        /*
         * Seek to the previous frame, and read the frame data.
         */
        offset = mpeg_time_seek_starttime(
                     ctx->mpegfp,
                     ctx->indxfp,
                     &ctx->stime,
                     &ctx->mpegtval,
                     ctx->has_xing);
        if (offset > 0) {
            _mpgedit_play_skip_frame(ctx);
        }

        /* Seek to specified location, should be next frame */
        ctx->stime.units = sec;
        ctx->stime.usec  = msec * 1000;
        mpeg_file_iobuf_clear(&ctx->mpegiobuf);
        offset = mpeg_time_seek_starttime(
                     ctx->mpegfp, 
                     ctx->indxfp, 
                     &ctx->stime, 
                     &ctx->mpegtval, 
                     ctx->has_xing);
    }
    else {
        /*
         * Just read the first frame into the prev_buffer the seek request
         * positions to the first frame of the file.
         */
        _mpgedit_play_skip_frame(ctx);
    }
    return offset;
}



void mpgedit_play_set_status_callback(void *_ctx,
                                      int (*status)(void *, long, long),
                                      void *data)
{
    playctx *ctx = MPGEDIT_PLAYCTX(_ctx);

    if (!ctx) {
        return;
    }
    ctx->fp_status = status;
    ctx->dp_status = data;
}



void mpgedit_play_decode_set_status_callback(
         void *_ctx,
         int (*status)(void *, unsigned char *, long, long, long),
         void *data)
{
    playctx *ctx = MPGEDIT_PLAYCTX(_ctx);

    if (!ctx) {
        return;
    }
    ctx->fp_decode_status = status;
    ctx->dp_decode_status = data;
}



int mpgedit_play_frame(void *_ctx)
{
    playctx          *ctx = MPGEDIT_PLAYCTX(_ctx);
    int              eof;
    int              found;
    long             sec;
    long             usec;
    unsigned char    *prevbuf;
    long             prevlen;

    if (!ctx) {
        return 0;
    }

    found = read_next_mpeg_frame(ctx->mpegfp, &ctx->mpegiobuf,
                                 &ctx->header, &eof);
    if (found) {
        if (!ctx->playfp) {
            if (ctx->mpctx) {
                /* 
                 * Close the mpeg audio decoder before calling decoder_new()
                 * which opens the decoder and audio device again.  On some
                 * platforms, the audio device can only be opened once, and
                 * a deadlock occurs without closing the audio device first.
                 */
                fp_mpgdecoder_close(ctx->mpctx);
            }
            ctx->playfp = 
                fp_mpgdecoder_new(ctx->mpctx, ctx->header.sample_rate, 
                                  (ctx->header.channel_mode != 3), 
                                  ctx->vol_left, ctx->vol_right);
            if (!ctx->playfp) {
                /*
                 * Minor problem.  Frame has already been read when we 
                 * determine that the audio decoder cannot be opened.
                 * However, need the frame to open the decoder. Catch 22.
                 */
                return 0;
            }
        }

        /*
         * Add frame time to running total. When the current time, tnow,
         * is advanced 1 second, call the notification callback.
         */
        mpeg_time_frame_increment(&ctx->stime, &ctx->header);
        mpeg_file_stats_gather(&ctx->stats, &ctx->header);
        ctx->frame_cnt++;
        mpeg_time_gettime(&ctx->stime, &sec, &usec);
        ctx->cursec  = sec;
        ctx->curusec = usec;
#if 1
        /*
         * adam/TBD:
         * These two lines imply these variables are equivalent.
         * Is this generally true throughout editif and playif??
         */
        ctx->stats_frame_header = ctx->header;
        ctx->mpegtval           = ctx->stime;
#endif

        if (ctx->seeked) {
            prevbuf = mpeg_file_iobuf_getptr(&ctx->prev_mpegiobuf);
            prevlen = ctx->prev_framesize;
            ctx->seeked = 0;
            mpeg_file_iobuf_clear(&ctx->prev_mpegiobuf);
        }
        else {
            prevbuf = NULL;
            prevlen = 0;
        }
        fp_mpgdecoder_play_skip_frame(
            ctx->mpctx, (void *) ctx->playfp,
            mpeg_file_iobuf_getptr(&ctx->mpegiobuf),
            ctx->header.frame_size, 
            prevbuf,
            prevlen,
            ctx->header.sample_rate,
            ctx->header.channel_mode != 3, NULL);

        mpeg_file_iobuf_setstart(&ctx->mpegiobuf,
                                 mpeg_file_iobuf_getstart(&ctx->mpegiobuf) +
                                 ctx->header.frame_size);
        if (ctx->fp_status) {
            found = ctx->fp_status(ctx->dp_status, sec, usec/1000);
        }

    }
    if (eof) {
        mpeg_file_iobuf_clear(&ctx->mpegiobuf);
    }
    return found;
}


/*
 * Advance over the current mp3 frame.  The current frame must be read, but
 * is not decoded or played.  The current frame's data is saved in 
 * `prev_mpegiobuf' for later use by mpgedit_play_frame().  This data is
 * required by the layer3 decoding engine `do_layer3()' function.
 */
static int _mpgedit_play_skip_frame(void *_ctx)
{
    playctx          *ctx = MPGEDIT_PLAYCTX(_ctx);
    int              eof;
    long             sec;
    long             usec;
    int              found;

    found = read_next_mpeg_frame(ctx->mpegfp, 
                                 &ctx->mpegiobuf, &ctx->header, &eof);
    if (found) {
        /*
         * Add frame time to running total. When the current time, tnow,
         * is advanced 1 second, call the notification callback.
         */
        mpeg_time_frame_increment(&ctx->stime, &ctx->header);
        mpeg_file_stats_gather(&ctx->stats, &ctx->header);
        ctx->frame_cnt++;
        mpeg_time_gettime(&ctx->stime, &sec, &usec);
        ctx->cursec  = sec;
        ctx->curusec = usec;
#if 1
        /*
         * adam/TBD:
         * These two lines imply these variables are equivalent.
         * Is this generally true throughout editif and playif??
         */
        ctx->stats_frame_header = ctx->header;
        ctx->mpegtval           = ctx->stime;
#endif

        mpeg_file_iobuf_clear(&ctx->prev_mpegiobuf);

#if 1
        memcpy(mpeg_file_iobuf_getptr(&ctx->prev_mpegiobuf),
               mpeg_file_iobuf_getptr(&ctx->mpegiobuf),
               ctx->header.frame_size);
#else
        /*
         * Layer 3 will only backskip up to 512 bytes, so
         * don't bother to copy more than that much data.
         * I wonder if this check and the pointer math is
         * more overhead than just copying the bytes?
         */
        if (ctx->header.frame_size > 512) {
            memcpy(mpeg_file_iobuf_getptr(&ctx->prev_mpegiobuf) +
                       ctx->header.frame_size - 512,
                   mpeg_file_iobuf_getptr(&ctx->mpegiobuf) + 
                       ctx->header.frame_size - 512,
                   512);
        }
        else {
            memcpy(mpeg_file_iobuf_getptr(&ctx->prev_mpegiobuf),
                   mpeg_file_iobuf_getptr(&ctx->mpegiobuf),
                   ctx->header.frame_size);
        }
#endif
        ctx->prev_framesize = ctx->header.frame_size;
        ctx->seeked = 1;

        mpeg_file_iobuf_setstart(&ctx->mpegiobuf,
                                 mpeg_file_iobuf_getstart(&ctx->mpegiobuf) +
                                 ctx->header.frame_size);
    }
    return ctx->seeked;
}


int mpgedit_play_skip_frame(void *_ctx)
{
    return _mpgedit_play_skip_frame(_ctx);
}


/* Do not free pcm_buf!  This is an alias into ctx */
int mpgedit_play_decode_frame(void *_ctx,
                              unsigned char **pcm_buf,
                              int *pcm_buf_len,
                              int *bsbytes)
{
    playctx          *ctx = MPGEDIT_PLAYCTX(_ctx);
    int              eof;
    int              found;
    long             sec;
    long             usec;
    unsigned char    *prevbuf;
    long             prevlen;

    if (!ctx) {
        return 0;
    }
    found = read_next_mpeg_frame(ctx->mpegfp, &ctx->mpegiobuf,
                                 &ctx->header, &eof);
    if (found) {
        /*
         * Add frame time to running total. When the current time, tnow,
         * is advanced 1 second, call the notification callback.
         */
        mpeg_time_frame_increment(&ctx->stime, &ctx->header);
        mpeg_file_stats_gather(&ctx->stats, &ctx->header);
        ctx->frame_cnt++;
        mpeg_time_gettime(&ctx->stime, &sec, &usec);
        ctx->cursec  = sec;
        ctx->curusec = usec;
#if 1
        /*
         * adam/TBD:
         * These two lines imply these variables are equivalent.
         * Is this generally true throughout editif and playif??
         */
        ctx->stats_frame_header = ctx->header;
        ctx->mpegtval           = ctx->stime;
#endif

        if (ctx->seeked) {
            ctx->seeked = 0;
            prevbuf = mpeg_file_iobuf_getptr(&ctx->prev_mpegiobuf);
            prevlen = ctx->prev_framesize;
        }
        else {
            prevbuf = NULL;
            prevlen = 0;
        }
        fp_mpgdecoder_decode_frame(
            ctx->mpctx,
            mpeg_file_iobuf_getptr(&ctx->mpegiobuf),
            ctx->header.frame_size, 
            prevbuf,
            prevlen,
            pcm_buf, pcm_buf_len, bsbytes);

#if 0
/*
 * adam/TBD:  There seems to be some difficulty with skip frame playback.  Not
 * really sure where the trouble is coming from. Just leaving this here for
 * now as a reminder.
 */
if (*bsbytes > prevlen) {
printf("\n\n  --> ERROR: mpgedit_play_decode_frame "
       "fr=%d bsbytes=%d prevlen=%ld\n\n",
       ctx->frame_cnt, *bsbytes, prevlen);
}
#endif
        mpeg_file_iobuf_setstart(&ctx->mpegiobuf,
                                 mpeg_file_iobuf_getstart(&ctx->mpegiobuf) +
                                 ctx->header.frame_size);
        if (ctx->fp_decode_status) {
            found = ctx->fp_decode_status(ctx->dp_decode_status, 
                                          *pcm_buf, *pcm_buf_len,
                                          sec, usec/1000);
        }
    }
    return found;
}


void mpgedit_play_close(void *_ctx)
{
    playctx *ctx = MPGEDIT_PLAYCTX(_ctx);

    if (!ctx) {
        return;
    }
    if (ctx->mpegfp) {
        fclose(ctx->mpegfp);
    }
    if (ctx->indxfp) {
        fclose(ctx->indxfp);
    }
    fp_mpgdecoder_close(ctx->mpctx);
    memset(ctx, 0, sizeof(*ctx));
    free(ctx);
}


long mpgedit_play_total_size(void *_ctx)
{
    return _ctx ? MPGEDIT_PLAYCTX(_ctx)->fsize : 0;
}


long mpgedit_play_total_sec(void *_ctx)
{
    return _ctx ? MPGEDIT_PLAYCTX(_ctx)->sec : 0;
}


long mpgedit_play_total_msec(void *_ctx)
{
    if (!_ctx) {
        return -1;
    }
    if (MPGEDIT_PLAYCTX(_ctx)->usec == -1) {
        return -1;
    }
    return MPGEDIT_PLAYCTX(_ctx)->usec/1000;
}


long mpgedit_play_current_sec(void *_ctx)
{
    return _ctx ? MPGEDIT_PLAYCTX(_ctx)->cursec : 0;
}


long mpgedit_play_current_msec(void *_ctx)
{
    if (!_ctx) {
        return -1;
    }
    if (MPGEDIT_PLAYCTX(_ctx)->curusec == -1) {
        return -1;
    }
    return MPGEDIT_PLAYCTX(_ctx)->curusec/1000;
}


void mpgedit_play_decode_frame_header_info(void *_ctx,
                                           int *mpeg_ver,
                                           int *mpeg_layer,
                                           int *bit_rate,
                                           int *sample_rate,
                                           int *samples_per_frame,
                                           int *frame_size,
                                           int *channel_mode,
                                           int *joint_ext_mode)
{
    playctx *ctx;

    if (!_ctx) {
        return;
    }
    ctx                    = MPGEDIT_PLAYCTX(_ctx);

    if (mpeg_ver) {
        *mpeg_ver          = ctx->header.mpeg_version;
    }
    if (mpeg_layer) {
        *mpeg_layer        = ctx->header.mpeg_layer;
    }
    if (bit_rate) {
        *bit_rate          = ctx->header.bit_rate;
    }
    if (sample_rate) {
        *sample_rate       = ctx->header.sample_rate;
    }
    if (samples_per_frame) {
        *samples_per_frame = ctx->header.samples_per_frame;
    }
    if (frame_size) {
        *frame_size        = ctx->header.frame_size;
    }
    if (channel_mode) {
        *channel_mode      = ctx->header.channel_mode;
    }
    if (joint_ext_mode) {
        *joint_ext_mode    = ctx->header.joint_ext_mode;
    }
}


int mpgedit_play_reset_audio(void *_ctx)
{
    playctx *ctx = MPGEDIT_PLAYCTX(_ctx);
    if (!ctx) {
        return -1;
    }

    return fp_mpgdecoder_reset_audio(ctx->mpctx);
}


/*
 * Set the left/right channel volume on the play context.
 * These values are used during the next call to mpgedit_play_frame().
 */
void mpgedit_play_volume_init(void *_ctx, int lvol, int rvol)
{
    playctx *ctx = MPGEDIT_PLAYCTX(_ctx);
    if (!ctx) {
        return;
    }
    ctx->vol_left  = lvol;
    ctx->vol_right = rvol;
}


/*
 * Get the current volume level set on the audio device.
 * this only works when the audio device is acutally open.
 */
int mpgedit_play_volume_get(void *_ctx, int *lvol, int *rvol)
{
    playctx *ctx = MPGEDIT_PLAYCTX(_ctx);
    if (!ctx) {
        return -1;
    }

    return fp_mpgdecoder_volume_get(ctx->mpctx, lvol, rvol);
}


/*
 * Set the current volume level set on the audio device.
 * this only works when the audio device is acutally open.
 * Warning: This call may not work as expected if audio playback
 * is not actually occuring when the call is made.  Use 
 * mpgedit_play_volume_init() to set the levels before playback is
 * happening.
 */
int mpgedit_play_volume_set(void *_ctx, int lvol, int rvol)
{
    playctx *ctx = MPGEDIT_PLAYCTX(_ctx);
    if (!ctx) {
        return -1;
    }

    return fp_mpgdecoder_volume_set(ctx->mpctx, lvol, rvol);
}



#if defined(UNIT_TEST)
int main(void)
{
    int go;
    void *ctx;
    int again;

    do {
        ctx = mpgedit_play_init("test1.mp3", 0);
        if (!ctx) {
            return 1;
        }
    } while (again);

    do {
        go = mpgedit_play_frame(ctx);
    } while (go);

    return 0;
}
#endif

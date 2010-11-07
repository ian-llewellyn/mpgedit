/*
 * mpgedit playback plugin using MPGLIB decoder library
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
/* 
 * This module is now portable across UNIX and Windows platforms, by
 * making use of the mpg123 audio interface abstraction.
 */

#ifndef lint
static char SccsId[] = "$Id: decoder_mpg123.c,v 1.15.2.1 2006/02/02 06:18:22 number6 Exp $";
#endif

/* 
 * API to access native win32 sound card interface, thanks to mpg123
 */
#include "audio.h"
#include "portability.h"

/*
 * mpglib interface header
 */
#include <interface.h>
#include <stdlib.h>
#include "decoder.h"

#ifdef WIN32
#define DEV_NULL "/nul"
#else
#define DEV_NULL "/dev/null"
#endif
#define MPGDECODER_BUFSZ 32
#define MPGDECODER_DEFAULT_SAMPLERATE 44100
#define MPGDECODER_BUFSIZ (MPGDECODER_BUFSZ * 1024)
/*
 * 48KHz stereo samples are 192000 bytes/sec.  Divide that by
 * 38 frames/sec and you get 5052 bytes. Round this up to 6K
 * for safety sake. Subtract 6K from the above BUFSIZ, to prevent
 * a buffer over run.  Use this as the threshold for flushing (playing)
 * the data to the sound device.
 */
#define MPGDECODER_MAXFILL ((MPGDECODER_BUFSZ-6) * 1024)


static void _mpgdecoder_free(mpgdecoder_ctx *ctx)
{
    mpgdecoder_ctx *mpctx = (mpgdecoder_ctx *) ctx;
    if (mpctx) {
        if (mpctx->g_outmpbuf) {
            free(mpctx->g_outmpbuf);
        }
        if (mpctx->g_mpctx) {
            free(mpctx->g_mpctx);
        }
        free(mpctx);
    }
}


void *mpgdecoder_alloc(void) 
{
    mpgdecoder_ctx *newctx = NULL;
    int sts = 1;
    
    newctx = calloc(1, sizeof(mpgdecoder_ctx));
    if (!newctx) {
        goto clean_exit;
    }

    /*
     * Hack:  This code does not work right when we allocate
     * just sizeof(MPSTR). Something in mpglib is using more
     * memory than it is supposed to.  The hack here is to 
     * allocate 2x what is needed.
     */
    newctx->g_mpctx = calloc(2, sizeof(MPSTR));
    if (!newctx->g_mpctx) {
        goto clean_exit;
    }
    newctx->g_outmpbuf = calloc(1, MPGDECODER_BUFSIZ);
    if (!newctx->g_outmpbuf) {
        goto clean_exit;
    }
    newctx->g_outmpbuflen = MPGDECODER_BUFSIZ;
    newctx->g_outmpcur = newctx->g_outmpbuf;
    
    sts = 0;

clean_exit:
    if (sts) {
        /*
         * Alloc error occurred; free everything
         */
        _mpgdecoder_free(newctx);
        newctx = NULL;
    }
    return newctx;
}

#if !defined(__linux)
#define FILENO_INDIRECT(a) *(a)
#else
#define FILENO_INDIRECT(a) a
#endif

/*
 * Assign stderr to /dev/null when request_fd == 0
 * Assign stderr to STDERR stream when request_fd == 2
 * Do nothing when request_fd is not 0 or 2.
 * Do nothing when requested stream assignment is the current state of stderr
 */
FILE *mpgedit_reopen_stderr(int request_fd)
{
    FILE *fp;
    int  stderr_fd;

    /*
     * Mac OS X when running xmpgedit from a dmg image from the UI does not
     * have stdin/out/err, so the operations performed by this function
     * are illegal.
     */
    if (fileno(stderr) != STDERR_FILENO) {
        return NULL;
    }
    if (request_fd != 0 && request_fd != 2) {
        /*
         * Do nothing when "request_fd" is not 0 or 2
         */
        return stderr;
    }

    stderr_fd = fileno(stderr);
    if (stderr_fd == 2) {
        if (request_fd == 2) {
            /*
             * Currently stderr is pointing at stderr and we request
             * stderr to point at stderr, so do nothing.
             */
            return stderr;
        }
        fp = fopen(DEV_NULL, "w+");
        if (!fp) {
            return NULL;
        }
        FILENO_INDIRECT(stderr) = FILENO_INDIRECT(fdopen(fileno(fp), "w+"));
    }
    else {
        if (request_fd == 0) {
            /*
             * stderr already pointing at /dev/null; return;
             */
            return stderr;
        }
        close(stderr_fd);
        FILENO_INDIRECT(stderr) = FILENO_INDIRECT(fdopen(2, "w+"));
    }
    return stderr;
}


void mpgdecoder_free(void *ctx)
{
    _mpgdecoder_free((mpgdecoder_ctx *) ctx);
}


void mpgdecoder_init(void *ctx)
{
    mpgdecoder_ctx *mpctx = (mpgdecoder_ctx *) ctx;
    if (mpctx && mpctx->g_mpctx) {
        InitMP3((MPSTR *) mpctx->g_mpctx);
#if 1
        /*
         * The current state of skip frame playback
         * of layer3 files no longer requires discarding
         * stderr warnings from mpglib.
         */
        /* Point stderr at /dev/null */
        mpgedit_reopen_stderr(0);
#endif
    }
}


void mpgdecoder_close(void *ctx)
{
    mpgdecoder_ctx *mpctx = (mpgdecoder_ctx *) ctx;
    struct audio_info_struct *ai =
        (struct audio_info_struct *) mpctx->g_dspctx;

    if (!ai) {
        return;
    }
    if (mpctx->g_outmpcur > mpctx->g_outmpbuf) {
        audio_play_samples(ai, mpctx->g_outmpbuf,
                           mpctx->g_outmpcur - mpctx->g_outmpbuf);

        mpctx->g_outmpcur = mpctx->g_outmpbuf;
    }

    /* Reset stderr to point at stderr again */
    mpgedit_reopen_stderr(2);

    audio_close(ai);
}

/*
 * Same as original mpgdecoder_open(), but takes volume 
 * initialization parameters.
 */
void *mpgdecoder_new(void *ctx, int sample_rate, int stereo, int lvol, int rvol)
{
    mpgdecoder_ctx *mpctx = (mpgdecoder_ctx *) ctx;
    int            sts;

    struct audio_info_struct *ai;

    ai = malloc(sizeof(struct audio_info_struct));
    if (!ai) {
        return NULL;
    }
    memset(ai, 0, sizeof(struct audio_info_struct));
    ai->format = AUDIO_FORMAT_SIGNED_16;
    if (sample_rate == 0) {
        sample_rate = MPGDECODER_DEFAULT_SAMPLERATE;
    }
    ai->rate     = sample_rate;
    ai->channels = stereo ? 2 : 1;
    if (lvol != -1 || rvol != -1) {
        if (lvol == -1) {
            lvol = rvol;
        }
        if (rvol == -1) {
            rvol = lvol;
        }
        ai->gain = ((rvol & 0xff) << 8) | (lvol & 0xff);
    }
    else {
        ai->gain     = -1;
    }
#if 1
    /*
     * The current state of skip frame playback
     * of layer3 files no longer requires discarding
     * stderr warnings from mpglib.
     */
    /* Point stderr at /dev/null */
    mpgedit_reopen_stderr(0);
#endif
    
    sts = audio_open(ai);
    if (sts == -1) {
        return NULL;
    }
    mpctx->g_sample_rate = sample_rate;
    mpctx->g_channels = ai->channels;
    mpctx->g_dspctx = (void *) ai;

    return mpctx->g_dspctx;
}


FILE *mpgdecoder_open(void *ctx, int sample_rate, int stereo)
{
    /* Ick, but must return non-NULL value to indicate success */
    return (FILE *) mpgdecoder_new(ctx, sample_rate, stereo, -1, -1);
}



void mpgdecoder_play_frame(void          *ctx,
                           FILE          **playfp,             /* not used */
                           unsigned char *buf,
                           int           len,
                           int           rate,
                           int           stereo)
{
    int                      done = 0;
    int                      iret = 0;
    int                      channels = stereo ? 2 : 1;
    mpgdecoder_ctx           *mpctx = (mpgdecoder_ctx *) ctx;
    struct audio_info_struct *ai =
        (struct audio_info_struct *) mpctx->g_dspctx;

    if (!ai) {
        return;
    }
    iret = decodeMP3(mpctx->g_mpctx, buf, len,
                     (char *) mpctx->g_outmpcur,
                     mpctx->g_outmpbuflen -
                       (mpctx->g_outmpcur - mpctx->g_outmpbuf),
                     &done);
    mpctx->g_outmpcur += done;
    if (iret == MP3_OK) {
        if (rate != mpctx->g_sample_rate || channels != mpctx->g_channels) {
            /*
             * Really want to just change the sound card playback sample rate.
             * However, must flush all pending data first, by closing
             * playback device.  It would not be good to change the
             * sample rate while data with the previous sample rate is
             * still in the sound card buffers.
             */
            audio_close(ai);
            InitMP3((MPSTR *) mpctx->g_mpctx);
            ai = (struct audio_info_struct *)
                mpgdecoder_open(ctx, rate, stereo);
            mpctx->g_dspctx = (void *) ai;
        }
        if (done + (mpctx->g_outmpcur - mpctx->g_outmpbuf) >=
            MPGDECODER_MAXFILL)
        {
            audio_play_samples(ai,
                               mpctx->g_outmpbuf,
                               mpctx->g_outmpcur - mpctx->g_outmpbuf);
            mpctx->g_outmpcur = mpctx->g_outmpbuf;
        }
    }
}


void mpgdecoder_play_skip_frame(
                           void          *ctx,
                           FILE          **playfp,             /* not used */
                           unsigned char *buf,
                           int           len,
                           unsigned char *prev_buf,
                           int           prev_len,
                           int           rate,
                           int           stereo)
{
    int                      done = 0;
    int                      iret = 0;
    int                      channels = stereo ? 2 : 1;
#if 1
int bsbytes;
#endif
    mpgdecoder_ctx           *mpctx = (mpgdecoder_ctx *) ctx;
    struct audio_info_struct *ai =
        (struct audio_info_struct *) mpctx->g_dspctx;

    if (!ai) {
        return;
    }
    iret = decodeMP3_9(mpctx->g_mpctx,
                       buf, len,
                       prev_buf, prev_len,
                       (char *) mpctx->g_outmpcur,
                       mpctx->g_outmpbuflen -
                         (mpctx->g_outmpcur - mpctx->g_outmpbuf),
                       &done, &bsbytes);
/* 
 * adam/TBD:  There seems to be some difficulty with skip frame playback.  Not
 * really sure where the trouble is coming from. Just leaving this here for
 * now as a reminder.
 */
#if 0
if (bsbytes > prev_len) {
printf("\n\n  --> ERROR: mpgedit_play_decode_frame "
"bsbytes=%d prevlen=%ld\n\n",
bsbytes, prev_len);
}
#endif
    mpctx->g_outmpcur += done;
    if (iret == MP3_OK) {
        if (rate != mpctx->g_sample_rate || channels != mpctx->g_channels) {
            /*
             * Really want to just change the sound card playback sample rate.
             * However, must flush all pending data first, by closing
             * playback device.  It would not be good to change the
             * sample rate while data with the previous sample rate is
             * still in the sound card buffers.
             */
            audio_close(ai);
            InitMP3((MPSTR *) mpctx->g_mpctx);
            ai = (struct audio_info_struct *)
                mpgdecoder_open(ctx, rate, stereo);
            mpctx->g_dspctx = (void *) ai;
        }
        if (done + (mpctx->g_outmpcur - mpctx->g_outmpbuf) >=
            MPGDECODER_MAXFILL)
        {
            audio_play_samples(ai,
                               mpctx->g_outmpbuf,
                               mpctx->g_outmpcur - mpctx->g_outmpbuf);
            mpctx->g_outmpcur = mpctx->g_outmpbuf;
        }
    }
}


char *mpgdecoder_version_06(void)
{
    static char version[] = "0.06";
    return version;
}


/*
 * Decode mp3 frame into PCM data.  Return status is always 0, which 
 * indicates this function is implemented.  pcm_buf is set to a valid
 * pointer value when the frame is successfully decoded, and pcm_buf_len
 * indicates the number of bytes present in this buffer.
 */
int mpgdecoder_decode_frame(void *ctx,
                            unsigned char *buf, int len,
                            unsigned char *prev_buf, int prev_len,
                            unsigned char **pcm_buf, int *pcm_buf_len,
                            int           *bsbytes)
{
    int                      done = 0;
    int                      iret = 0;
    mpgdecoder_ctx           *mpctx = (mpgdecoder_ctx *) ctx;

    iret = decodeMP3_9(mpctx->g_mpctx, 
                       buf, len,
                       prev_buf, prev_len,
                       (char *) mpctx->g_outmpbuf,
                       mpctx->g_outmpbuflen, &done, bsbytes);
    if (iret == MP3_OK) {
        *pcm_buf     = mpctx->g_outmpbuf,
        *pcm_buf_len = done;
    }
    else {
        *pcm_buf     = NULL;
        *pcm_buf_len = 0;
    }
    return 0;
}


int mpgdecoder_reset_audio(void *ctx)
{
    mpgdecoder_ctx *mpctx = (mpgdecoder_ctx *) ctx;
    struct audio_info_struct *ai =
        (struct audio_info_struct *) mpctx->g_dspctx;

    if (!ai) {
        return -1;
    }
    return audio_reset_parameters(ai);
}


int mpgdecoder_volume_get(void *ctx, int *lvol, int *rvol)
{
    mpgdecoder_ctx *mpctx = (mpgdecoder_ctx *) ctx;
    struct audio_info_struct *ai = NULL;
    int mixer_val;
    int sts;

    if (!mpctx) {
        return -1;
    }
    ai = (struct audio_info_struct *) mpctx->g_dspctx;

    sts = audio_get_gain(ai);
    if (sts != 0) {
        return sts;
    }
    mixer_val = ai->gain;
    *lvol     = mixer_val & 0xff;
    *rvol     = (mixer_val >> 8) & 0xff;
    return 0;
}


int mpgdecoder_volume_set(void * ctx, int lvol, int rvol)
{
    mpgdecoder_ctx *mpctx = (mpgdecoder_ctx *) ctx;
    struct audio_info_struct *ai = NULL;

    if (!mpctx) {
        return -1;
    }
    ai = (struct audio_info_struct *) mpctx->g_dspctx;
    if (!ai) {
        return -1;
    }

    if (lvol != -1 || rvol != -1) {
        lvol = (lvol < 0)   ? 0     : lvol;
        lvol = (lvol > 100) ? 100   : lvol;
        rvol = (rvol < 0)   ? 0     : rvol;
        rvol = (rvol > 100) ? 100   : rvol;
        ai->gain = ((rvol & 0xff) << 8) | (lvol & 0xff);
    }
    else {
        ai->gain = -1;
    }

    return audio_set_gain(ai);
}

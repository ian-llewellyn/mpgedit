/*
 * mpgedit playback plugin using MAD decoder library
 *
 * Copyright (C) 2005-2006 Adam Bernstein
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
static char SccsId[] = "";
#endif

/* 
 * API to access native win32 sound card interface, thanks to mpg123
 */
#include "audio.h"
#include "portability.h"
#include <limits.h>
#include <mad.h>

/*
 * mpglib interface header
 */
#if 0
 /* mpglib interface */
#include <interface.h>
#endif
#include <stdlib.h>
#include "decoder.h"

#if defined (__macosx)
#define SWAP_16(v) { \
    unsigned short t = (unsigned short) (v); \
    unsigned char *o = (unsigned char *) &(v); \
    unsigned char *i = (unsigned char *) &t; \
    o[0] = i[1]; \
    o[1] = i[0]; \
}
#else
#define SWAP_16(v)
#endif


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
#define OUTPUT_BUFFER_SIZE 8192 /* Must be an integer multiple of 4. */
#define MAX_MP3_SIZE 2048       /* Slightly larger than max MP3 frame of 2016 bytes */
#define STREAM_BUF_SIZE  (MAX_MP3_SIZE * 3)


#define MadFixedToSshort(Fixed) \
    ((Fixed) >= MAD_F_ONE)  ? SHRT_MAX  : \
    ((Fixed) <= -MAD_F_ONE) ? -SHRT_MAX : ((signed short)((Fixed)>>(MAD_F_FRACBITS-15)))



typedef struct _mad_decoder_ctx_t {
    unsigned char     stream_buf[STREAM_BUF_SIZE];
    unsigned char     OutputBuffer[OUTPUT_BUFFER_SIZE];
    unsigned char     *OutputPtr;
    unsigned char     *OutputBufferEnd;
    int               sbi;                               /* stream_buf index */
    int               frame_count;
    int               am_syncing;

#if 0
    int               verbose;
    int               silent; 
    char              *string;
    FILE              *infp;
    FILE              *outfp;
    mpeg_header_data  stats_header;
    mpeg_file_iobuf   mpegiobuf;
#endif

    struct mad_stream Stream;
    struct mad_frame  Frame;
    struct mad_synth  Synth;
    mad_timer_t       Timer;
} mad_decoder_ctx_t;


#if 0
typedef struct _mpgdecoder_ctx {
    void          *g_mpctx;     /* MPSTR structure */
    void          *g_dspctx;    /* audio playback context */
    unsigned char *g_outmpbuf;  /* 5K buffer size */
    unsigned char *g_outmpcur;  /* Current offset into buffer */
    int           g_alloclen;   /* Buffer allocated size */
    int           g_outmpbuflen;
    int           g_sample_rate;
    int           g_channels;
} mpgdecoder_ctx;
#endif

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


#if 0
int init_mp3_data_buffers(char *outfile, mpgedit_madbuf_t *ctx)
{
    memset(ctx, 0, sizeof(*ctx));

    ctx->string          = malloc(1024);
    if (!ctx->string) {
        return 1;
    }
    ctx->OutputPtr       = ctx->OutputBuffer;
    ctx->OutputBufferEnd = ctx->OutputPtr + sizeof(ctx->OutputBuffer);
    if (outfile[0] == '-' && outfile[1] == '\0') {
        ctx->outfp = stdout;
    }
    else {
        ctx->outfp = fopen(outfile, "wb");
    }
    return ctx->outfp ? 0 : 1;
}
#endif


void *mpgdecoder_alloc(void) 
{
    mpgdecoder_ctx *newctx = NULL;
    mad_decoder_ctx_t *madctx;
    int sts = 1;
    
    newctx = calloc(1, sizeof(mpgdecoder_ctx));
    if (!newctx) {
        goto clean_exit;
    }

    madctx = (mad_decoder_ctx_t *) calloc(1, sizeof(*madctx));
    if (!madctx) {
        goto clean_exit;
    }
    newctx->g_mpctx = madctx;
    madctx->OutputPtr = madctx->OutputBuffer;
    madctx->OutputBufferEnd = madctx->OutputPtr + sizeof(madctx->OutputBuffer);
    
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
    mpgdecoder_ctx    *mpctx = (mpgdecoder_ctx *) ctx;
    mad_decoder_ctx_t *madctx;

    
    if (mpctx && mpctx->g_mpctx) {
        madctx = (mad_decoder_ctx_t *) mpctx->g_mpctx;
        mad_stream_init(&madctx->Stream);
        mad_frame_init(&madctx->Frame);
        mad_synth_init(&madctx->Synth);
        mad_timer_reset(&madctx->Timer);



#if 0
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

#if 0
    /* Reset stderr to point at stderr again */
    mpgedit_reopen_stderr(2);
#endif

    audio_close(ai);
}


FILE *mpgdecoder_open(void *ctx, int sample_rate, int stereo)
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
    ai->gain     = -1;
    
    sts = audio_open(ai);
    if (sts == -1) {
        printf("audio_open failed\n");
        return NULL;
    }
    mpctx->g_sample_rate = sample_rate;
    mpctx->g_channels = ai->channels;
    mpctx->g_dspctx = (void *) ai;

#if 0
    /*
     * The current state of skip frame playback
     * of layer3 files no longer requires discarding
     * stderr warnings from mpglib.
     */
    /* Point stderr at /dev/null */
    mpgedit_reopen_stderr(0);
#endif

    /* Ick, but must return non-NULL value to indicate success */
    return (FILE *) mpctx->g_dspctx;
}


static void fill_decoder_data_buffers(unsigned char *buf, int len, 
                                      mad_decoder_ctx_t *ctx)
{
    if (ctx->sbi == 0) {
        memcpy(ctx->stream_buf, buf, len);
        ctx->sbi = len;
    }
    else if (ctx->sbi < STREAM_BUF_SIZE) {
        memcpy(&ctx->stream_buf[ctx->sbi], buf, len);
        ctx->sbi += len;
    }
}


int decode_mp3_stream(mad_decoder_ctx_t *ctx)
{
    int rsts;
    int error = -1;
    int adjust_stream = 0;

    mad_stream_buffer(&ctx->Stream, ctx->stream_buf, ctx->sbi);
    rsts = mad_frame_decode(&ctx->Frame, &ctx->Stream);
    if (rsts) {
        if (MAD_RECOVERABLE(ctx->Stream.error)) {
            if (ctx->Stream.error == MAD_ERROR_BADDATAPTR ||
                ctx->Stream.error == MAD_ERROR_LOSTSYNC)
            {
                memmove(ctx->stream_buf, ctx->stream_buf + 1, ctx->sbi - 1);
                ctx->sbi--;
                error = MAD_ERROR_LOSTSYNC;
            }
            else {
                adjust_stream = 1;
                error = 0;
            }
        }
        else if (ctx->Stream.error == MAD_ERROR_BUFLEN) {
            error = MAD_ERROR_BUFLEN;
            adjust_stream = 1;
        }
    }
    else {
        error = 0;
        adjust_stream = 1;
    }

    if (adjust_stream) {
        if (ctx->Stream.next_frame) {
            ctx->sbi = ctx->Stream.bufend - ctx->Stream.next_frame;
            memcpy(ctx->stream_buf, ctx->Stream.next_frame, ctx->sbi);
        }
        else {
            ctx->sbi = 0;
        }
    }
    return error;
}



int output_synth_data(mad_decoder_ctx_t *ctx, struct audio_info_struct *ai)
{
    int i;

    for(i=0;i<ctx->Synth.pcm.length;i++) {
        signed short    Sample;

        /* Left channel */
        Sample=MadFixedToSshort(ctx->Synth.pcm.samples[0][i]);
        SWAP_16(Sample);
        *(ctx->OutputPtr++) = Sample & 0xff;
        *(ctx->OutputPtr++) = Sample >> 8;

        /* Right channel. If the decoded stream is monophonic then
         * the right output channel is the same as the left one.
         */
        if(MAD_NCHANNELS(&ctx->Frame.header)==2) {
            Sample=MadFixedToSshort(ctx->Synth.pcm.samples[1][i]);
            SWAP_16(Sample);
            *(ctx->OutputPtr++) = Sample & 0xff;
            *(ctx->OutputPtr++) = Sample >> 8;
        }

        /* Flush the output buffer if it is full. */
        if (ctx->OutputPtr >= ctx->OutputBufferEnd) {
            audio_play_samples(ai, ctx->OutputBuffer, OUTPUT_BUFFER_SIZE);
            ctx->OutputPtr = ctx->OutputBuffer;
        }
    }
    return 0;
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
    int                      channels = stereo ? 2 : 1;
    int                      rsts;
    mpgdecoder_ctx           *mpctx = (mpgdecoder_ctx *) ctx;
    mad_decoder_ctx_t        *madctx = (mad_decoder_ctx_t *) mpctx->g_mpctx;
    struct audio_info_struct *ai = (struct audio_info_struct *) mpctx->g_dspctx;

    if (!ai) {
        return;
    }

    if (madctx->sbi < STREAM_BUF_SIZE/2) {
        fill_decoder_data_buffers(buf, len, madctx);
    }

    if (madctx->sbi < STREAM_BUF_SIZE/2) {
        return;
    }

    rsts = decode_mp3_stream(madctx);
    while (rsts == MAD_ERROR_LOSTSYNC) {
        rsts = decode_mp3_stream(madctx);
    }

    while (rsts == 0) {
        mad_synth_frame(&madctx->Synth, &madctx->Frame);
        if (rate != mpctx->g_sample_rate || channels != mpctx->g_channels) {
            /*
             * Really want to just change the sound card playback sample
             * rate.  However, must flush all pending data first, by
             * closing playback device.  It would not be good to change
             * the sample rate while data with the previous sample rate is
             * still in the sound card buffers.
             */
            audio_close(ai);
            ai = (struct audio_info_struct *)
                mpgdecoder_open(ctx, rate, stereo);
            mpctx->g_dspctx = (void *) ai;
        }
        output_synth_data(madctx, ai);
        rsts = decode_mp3_stream(madctx);
    }
}



void mpgdecoder_play_frame(void          *ctx,
                           FILE          **playfp,             /* not used */
                           unsigned char *buf,
                           int           len,
                           int           rate,
                           int           stereo)
{
    mpgdecoder_play_skip_frame(ctx, playfp, buf, len, NULL, 0, rate, stereo);
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











#if 0
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
#endif
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

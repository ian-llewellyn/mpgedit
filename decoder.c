/*
 * mpgedit playback plugin using popen
 *
 * Copyright (C) 2001, 2005-2006 Adam Bernstein
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

#ifndef lint
static char SccsId[] = "$Id: decoder.c,v 1.13 2005/11/19 18:30:38 number6 Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include "decoder.h"

#ifndef _NO_STATIC
#define _STATIC static
#else
#define _STATIC
#endif

_STATIC void *mpgdecoder_alloc(void)
{
    return calloc(1, sizeof(mpgdecoder_ctx));
}



_STATIC void mpgdecoder_free(void *ctx)
{
    if (ctx) {
        free(ctx);
    }
}

_STATIC void mpgdecoder_init(void *ctx)
{
}


_STATIC void *mpgdecoder_new(void *ctx,
                             int sample_rate, int stereo,
                             int lvol, int rvol)
{
    FILE *playfp;
    mpgdecoder_ctx *mpctx;

    if (!ctx) {
        return NULL;
    }
    mpctx = (mpgdecoder_ctx *) ctx;
    playfp = popen("mp3decoder.sh", "w");
    mpctx->g_dspctx = (void *) playfp;
    return (void *) playfp;
}

_STATIC FILE *mpgdecoder_open(void *ctx, int sample_rate, int stereo)
{
    return (FILE *) mpgdecoder_new(ctx, sample_rate, stereo, -1, -1);
}

_STATIC void mpgdecoder_close(void *ctx)
{
    FILE *playfp;
    mpgdecoder_ctx *mpctx;

    if (!ctx) {
        return;
    }
    mpctx = (mpgdecoder_ctx *) ctx;
    playfp = (FILE *) mpctx->g_dspctx;
    if (playfp) {
        fclose(playfp);
    }
    mpctx->g_dspctx = NULL;
}


_STATIC void mpgdecoder_play_frame(void          *ctx,
                           FILE          **playfp,
                           unsigned char *buf,
                           int           len,
                           int           rate,
                           int           stereo)
{
    FILE *fp;
    mpgdecoder_ctx *mpctx;

    if (!ctx) {
        return;
    }
    mpctx = (mpgdecoder_ctx *) ctx;
    fp = (FILE *) mpctx->g_dspctx;
    if (fp) {
        fwrite(buf, len, 1, fp);
    }
}


_STATIC char *mpgdecoder_version_06(void)
{
    static char version[] = "0.06";

    return version;
}


void *default_mpgdecoder_alloc(void)
{
    return mpgdecoder_alloc();
}


void default_mpgdecoder_free(void *ctx)
{
    mpgdecoder_free(ctx);
}


void default_mpgdecoder_init(void *ctx)
{
    mpgdecoder_init(ctx);
}


FILE *default_mpgdecoder_open(void *ctx, int sample_rate, int stereo)
{
    return mpgdecoder_open(ctx, sample_rate, stereo);
}

void default_mpgdecoder_close(void *ctx)
{
    mpgdecoder_close(ctx);
}

void default_mpgdecoder_play_frame(void          *ctx,
                                   FILE          **playfp,
                                   unsigned char *buf,
                                   int           len,
                                   int           rate,
                                   int           stereo)
{
    mpgdecoder_play_frame(ctx, playfp, buf, len, rate, stereo);
}

char *default_mpgdecoder_version_06(void)
{
    return mpgdecoder_version_06();
}

int default_mpgdecoder_decode_frame(void *ctx,
                                    unsigned char *buf, int len,
                                    unsigned char *prev_buf, int prev_len,
                                    unsigned char **pcm_buf,
                                    int           *pcm_buf_len,
                                    int           *bsbytes)
{
    return -1;
}


void default_mpgdecoder_play_skip_frame(
                                   void          *ctx,
                                   FILE          **playfp,
                                   unsigned char *buf,
                                   int           len,
                                   unsigned char *prev_buf,
                                   int           prev_len,
                                   int           rate,
                                   int           stereo,
                                   int           *bsbytes)
{
    mpgdecoder_play_frame(ctx, playfp, buf, len, rate, stereo);
}


int default_mpgdecoder_reset_audio(void * ctx)
{
    return -1;
}


int default_mpgdecoder_volume_get(void * ctx, int *lvol, int *rvol)
{
    return -1;
}


int default_mpgdecoder_volume_set(void * ctx, int lvol, int rvol)
{
    return -1;
}

void *default_mpgdecoder_new(void *ctx,
                             int sample_rate,
                             int stereo,
                             int rvol,
                             int lvol)
{
    return mpgdecoder_new(ctx, sample_rate, stereo, rvol, lvol);
}

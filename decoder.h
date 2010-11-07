/*
 * audio playback decoder header file
 *
 * Copyright (C) 2001-2006 Adam Bernstein
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
 * SccsId[] = "$Id: decoder.h,v 1.10 2005/11/19 18:30:38 number6 Exp $"
 */

#ifndef _DECODER_H
#define _DECODER_H

#include <stdio.h>
#include "portability.h"
 
#ifdef _NO_STATIC
_DSOEXPORT void *mpgdecoder_alloc(void);
_DSOEXPORT void mpgdecoder_free(void *ctx);
_DSOEXPORT void mpgdecoder_init(void *ctx);
_DSOEXPORT FILE *mpgdecoder_open(void *ctx, int sample_rate, int stereo);
_DSOEXPORT void mpgdecoder_close(void *ctx);
_DSOEXPORT void mpgdecoder_play_frame(void *ctx, FILE **playfp,
                                      unsigned char *buf, int len,
                                      int rate, int stereo);
_DSOEXPORT void mpgdecoder_play_skip_frame(void *ctx, FILE **playfp,
                                      unsigned char *buf, int len,
                                      unsigned char *prev_buf, int prev_len,
                                      int rate, int stereo);
_DSOEXPORT int mpgdecoder_decode_frame(void *ctx,
                                       unsigned char *buf, int len,
                                       unsigned char *prev_buf, int prev_len,
                                       unsigned char **pcm_buf,
                                       int *pcm_buf_len, int *bsbytes);
_DSOEXPORT char *mpgdecoder_version_06(void);
_DSOEXPORT void *mpgdecoder_new(void *ctx, 
                                int sample_rate, int stereo, 
                                int rvol, int lvol);
_DSOEXPORT int mpgdecoder_volume_get(void *, int *, int *);
_DSOEXPORT int mpgdecoder_volume_set(void *, int,  int);

#endif /* _NO_STATIC */

void *default_mpgdecoder_alloc(void);
void default_mpgdecoder_free(void *ctx);
void default_mpgdecoder_init(void *ctx);
FILE *default_mpgdecoder_open(void *ctx, int sample_rate, int stereo);
void default_mpgdecoder_close(void *ctx);
void default_mpgdecoder_play_frame(void *ctx, FILE **playfp,
                           unsigned char *buf, int len, int rate, int stereo);
char *default_mpgdecoder_version_06(void);

int default_mpgdecoder_decode_frame(void *ctx,
                                    unsigned char *buf, int len,
                                    unsigned char *prev_buf, int prev_len,
                                    unsigned char **pcm_buf,
                                    int *pcm_buf_len, int *bsbytes);
void default_mpgdecoder_play_skip_frame(void *ctx, FILE **playfp,
                           unsigned char *buf, int len, 
                           unsigned char *prev_buf, int prev_len, 
                           int rate, int stereo, int *bsbytes);

int default_mpgdecoder_reset_audio(void *ctx);
int default_mpgdecoder_volume_get(void * ctx, int *lvol, int *rvol);
int default_mpgdecoder_volume_set(void * ctx, int lvol, int rvol);
void *default_mpgdecoder_new(void *ctx,
                             int sample_rate,
                             int stereo,
                             int rvol,
                             int lvol);

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

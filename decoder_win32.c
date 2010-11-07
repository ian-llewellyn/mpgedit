/*
 * mpgedit win32 playback plugin using MPGLIB decoder library
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

#ifndef lint
static char SccsId[] = "$Id: decoder_win32.c,v 1.8 2005/11/19 18:30:39 number6 Exp $";
#endif

/*
 * mpglib interface header
 */
#include <stdlib.h>
#include "decoder.h"



void *default_mpgdecoder_alloc(void)
{
    return NULL;
}


void default_mpgdecoder_free(void *ctx)
{
}


void default_mpgdecoder_init(void *ctx)
{
}


FILE *default_mpgdecoder_open(void *ctx, int sample_rate, int stereo)
{
    return NULL;
}


void default_mpgdecoder_close(void *ctx)
{
}


void default_mpgdecoder_play_frame(void          *ctx,
                                   FILE          **playfp,
                                   unsigned char *buf,
                                   int           len,
                                   int           rate,
                                   int           stereo)
{
}


char *default_mpgdecoder_version_06(void)
{
    static char version[] = "0.06";

    return version;
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
}


int default_mpgdecoder_reset_audio(void *ctx)
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
    return NULL;
}

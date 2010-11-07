/*
 * Time index file generation test program
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
static char SccsId[] = "$Id: mpeg_indx.c,v 1.5 2004/07/19 14:44:04 number6 Exp $";
#endif

#include <stdio.h>
#include "mpegindx.h"
#include <stdarg.h>
#include "mpegfio.h"
#include "p_playif.h"


static void ttyio_printf(void *ctx, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    fflush(stdout);
    va_end(ap);
}


int main(int argc, char *argv[])
{
    FILE                *infp;
    FILE                *indxfp;
    mpegfio_iocallbacks ttyio;
    void                *ifctx;
    int                 eof;
    long                tlast = 0;
    long                tnow  = 0;

    if (argc == 1) {
        printf("usage: %s input_mp3_name\n", argv[0]);
        return 1;
    }

    memset(&ttyio, 0, sizeof(ttyio));
    ttyio.printf = ttyio_printf;

    infp = fopen(argv[1], "rb");
    if (!infp) {
        perror("fopen");
        return 1;
    }
    indxfp = fopen("indx.out", "wb");
    if (!indxfp) {
        perror("fopen(indx.out)");
        return 1;
    }
    ifctx = calloc(1, sizeof(playctx));
    do {
        eof = mp3edit_create_indexfile(infp, indxfp, ifctx, NULL);
        tnow = mp3edit_indexfile_sec(ifctx);
        if (tnow > tlast) {
            printf("\rElapsed time: %4ld:%02ld| %5lds|Frame: %-7ld\r",
                   tnow/60,
                   tnow%60,
                   tnow,
                   mp3edit_indexfile_frames(ifctx));
            fflush(stdout);
        
            tlast = tnow;
        }
    } while (!eof);
    return 0;
}

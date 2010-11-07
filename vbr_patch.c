/*
 * XING header editing test program
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
static char SccsId[] = "$Id: vbr_patch.c,v 1.5 2005/11/27 06:40:32 number6 Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xing_header.h"
#include "mpegfio.h"
#include "header.h"


/*
 * Simple utility to display contents of an MP3 VBR header, and patch
 * the frames and bytes fields.  This type of fixup is needed if you have 
 * cut a portion of a VBR file, and then appended it onto the end of 
 * another file.
 */
int main(int argc, char *argv[])
{
    FILE          *fp;
    int           header_size;
    XHEADDATA     X;
    int           i;
    int           frame_add = 0;
    int           bytes_add = 0;
    int           frames = 0;
    int           bytes = 0;
    char          *filename;
    char          *cp;
    char          *str;
    mpeg_header_data mpeg_header;

    memset(&X, 0, sizeof(X));

    if (argc == 1 || argc > 4) {
        printf("usage: %s mp3_file [[+]frames] [[+]bytes]\n", argv[0]);
        return 1;
    }
    filename = argv[1];

    if (argc > 2) {
        cp = argv[2];
        if (*cp == '+') {
            frame_add = 1;
            cp++;
        }
        frames = atoi(cp);
    }
    if (argc == 4) {
        cp = argv[3];
        if (*cp == '+') {
            bytes_add = 1;
            cp++;
        }
        bytes = atoi(cp);
    }

    if (argc == 2) {
        fp = fopen(filename, "rb");
    }
    else {
        fp = fopen(filename, "rb+");
    }
    if (!fp) {
        perror("fopen");
        return 1;
    }
    memset(&mpeg_header, sizeof(mpeg_header), 0);
    header_size = mpegfio_has_xing_header(fp, &mpeg_header, &X, 0);
    if (header_size == 0) {
        fprintf(stderr, "File does not have XING header\n");
        return 1;
    }

    /*
     * Hex dump the first 64 bytes of the header
     */
    for (i = 0; i < 64; i++) {
        if ((i%16) == 0) {
            printf("\n");
        }
        printf("%02x ", X.xingbuf[i]);
    }
    printf("\n");

    str = malloc(1024);
    if (!str) {
        return 1;
    }
    xingheader2str(&X, str);
    printf("%s", str);
    printf("xing size = %d\n", header_size);
    printf("==================================\n");

    if (frame_add) {
        frames += X.frames;
    }
    if (bytes_add) {
        bytes += X.bytes;
    }
    if (frames) {
        if (bytes == 0) {
            bytes = X.bytes;
        }

        xingheader_edit(&X, frames, bytes, X.h_id, X.h_mode);
        if (xingheader_init(X.xingbuf, header_size, &X)) {
            xingheader2str(&X, str);
            printf("%s", str);
            printf("==================================\n");
            fseek(fp, 0, SEEK_SET);
            fwrite(X.xingbuf, header_size, 1, fp);
        }
        else {
            printf("Did not find Xing header\n");

        }
        fclose(fp);
    }
    free(str);
    return 0;
}

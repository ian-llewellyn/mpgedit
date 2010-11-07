/*
 * MPEG audio frame header decode test program
 *
 * Copyright (C) 2004 Adam Bernstein
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


#include "header.h"
#include "mpegfio.h"
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    unsigned char buf[4];
    mpeg_header_data header;
    char *str;
    mpeg_file_iobuf mpegiobuf;
    mpeg_header_data stats_header;
    int eof;
    int found;
    FILE *infp;


    str = malloc(1024);
    if (!str) {
        return 1;
    }

    if (argc == 1) {
        printf("usage: readheader mp3_file\n");
        return 1;
    }

    infp = mpeg_file_open(argv[1], "rb");
    if (!infp) {
        perror("fopen failed");
        return 1;
    }
    eof = 0;
    memset(&header, 0, sizeof(header));
    mpeg_file_iobuf_clear(&mpegiobuf);
    memset(&stats_header, 0, sizeof(stats_header));
    found = mpeg_file_next_frame_read(infp, &mpegiobuf, &stats_header, &eof);
    if (found && !eof) {
        memcpy(&buf, mpeg_file_iobuf_getptr(&mpegiobuf), 4);

        if (mpgedit_header_decode(buf, &header, MPGEDIT_ALLOW_MPEG1L1)) {
            mpeg_header_values2str(&header, str);
            printf("%s", str);
        }
        free(str);
    }
    mpeg_file_close(infp);
    return 0;
}

/*
 * MPEG audio frame header decode test program
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
static char SccsId[] = "$Id: tstheader.c,v 1.5.6.1 2009/04/02 03:14:02 number6 Exp $";
#endif

#include "header.h"
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    unsigned char buf[] = {0xff, 0xfb, 0xb0, 0x04};
    mpeg_header_data header;
    char *str;

    str = malloc(1024);
    if (!str) {
        return 1;
    }

    if (argc > 4) {
        buf[0] = (unsigned char) strtol(argv[1], NULL, 16);
        buf[1] = (unsigned char) strtol(argv[2], NULL, 16);
        buf[2] = (unsigned char) strtol(argv[3], NULL, 16);
        buf[3] = (unsigned char) strtol(argv[4], NULL, 16);
    }

    memset(&header, 0, sizeof(header));
    if (decode_mpeg_header(buf, &header, MPGEDIT_ALLOW_MPEG1L1)) {
        mpeg_header_values2str(&header, str);
        printf("%s", str);
    }
    free(str);
    return 0;
}

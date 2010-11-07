/*
 * mp3time test program
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
static char SccsId[] = "$Id: mp3_time.c,v 1.5 2005/11/27 06:40:32 number6 Exp $";
#endif

#include "mp3time.h"
#include "header.h"

int main(void)
{
    int i;
    char *str;
    mpeg_time tval;
    unsigned char buf[] = {0xff, 0xfb, 0xb0, 0x04};
    mpeg_header_data header;

    str = malloc(1024);
    if (!str) {
        return 1;
    }

    memset(&header, 0, sizeof(header));
    if (decode_mpeg_header(buf, &header, 0)) {
        mpeg_header_values2str(&header, str);
        printf("%s", str);
    }

    mpeg_time_clear(&tval);
    for (i=0; mpeg_time_2seconds(&tval) < 20; i++) {
        mpeg_time_frame_increment(&tval, &header);
        printf("%ld\n", mpeg_time_2seconds(&tval));
    }
    free(str);
    return 0;
}

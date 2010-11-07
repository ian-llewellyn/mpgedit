/*
 * Example program that uses libmd5 functions.
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

#include <stdio.h>
#include "md5.h"

int main(int argc, char *argv[])
{
    FILE        *fp = NULL;
    int         len;
    int         i;
    char        buf[1024];
    md5_state_t md5;
    md5_byte_t  sum[16];

    md5_init(&md5);
    if (argc == 1) {
        fp = stdin;
    }
    else {
        fp = fopen(argv[1], "rb");
        if (!fp) {
            perror("fopen");
            return 1;
        }
    }

    while (!feof(fp)) {
        len = fread(buf, 1, sizeof(buf), fp);
        if (len != -1) {
            md5_append(&md5, (const md5_byte_t *) buf, len);
        }
    }

    md5_finish(&md5, sum);
    for (i=0; i<sizeof(sum); i++) {
        printf("%02x", (unsigned char) sum[i]);
    }
    printf("\n");
    return 0;
}

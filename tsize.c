/*
 * Test program that acquires total mp3 file size from index file.
 *
 * Copyright (C) 2002 Adam Bernstein
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

#include "playif.h"

int main(int argc, char *argv[])
{
    void *ctx;

    if (argc == 1) {
        printf("%s: mpg_file\n", argv[0]);
        return 1;
    }
    
    ctx = mpgedit_play_init(argv[1], 0);
    if (ctx) {
        printf("total file size %ld\n", mpgedit_play_total_size(ctx));
        printf("total sec       %ld\n", mpgedit_play_total_sec(ctx));
        printf("total msec      %ld\n", mpgedit_play_total_msec(ctx));
        mpgedit_play_close(ctx);
    }

    return 0;
}

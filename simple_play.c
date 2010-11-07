/*
 * Simple mp3 player using playif API
 *
 * Copyright (C) 2003 Adam Bernstein
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
#include "playif.h"
#include "editif.h"

int main(int argc, char *argv[])
{
    void *ctx;
    long cur_sec=0, past_sec=0;

    if (argc == 1) {
        fprintf(stderr, "usage: %s mp3_file\n", argv[0]);
        return 1;
    }

    ctx = mpgedit_edit_index_init(argv[1]);
    while (!mpgedit_edit_index(ctx));
    mpgedit_edit_index_free(ctx);

    ctx = mpgedit_play_init(argv[1], 0);
    if (!ctx) {
        return 1;
    }

    mpgedit_play_volume_init(ctx, VOLUME_DEFAULT_VALUE, VOLUME_DEFAULT_VALUE);

    if (mpgedit_play_total_size(ctx) != -1) {
        printf("total file size: %ld | ", mpgedit_play_total_size(ctx));
    }
    if (mpgedit_play_total_sec(ctx) != -1) {
        printf("total play time: %ld.%03lds",  
               mpgedit_play_total_sec(ctx),
               mpgedit_play_total_msec(ctx));
    }
    printf("\n");

    while (mpgedit_play_frame(ctx)) {
        cur_sec = mpgedit_play_current_sec(ctx);
        if (cur_sec != past_sec) {
            past_sec = cur_sec;
            printf("%ld.%03lds\n", cur_sec, mpgedit_play_current_msec(ctx));
        }
    }
    printf("%ld.%03lds\n", mpgedit_play_current_sec(ctx),
                           mpgedit_play_current_msec(ctx));
    mpgedit_play_close(ctx);

    return 0;
}

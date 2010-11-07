/*
 * mpgedit curses helper utility functions.
 *
 * Copyright (C) 2001-2006 Adam Bernstein. All Rights Reserved.
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

#include "portability.h"
#include "playif.h"
#include "editif.h"
#include "volumeif.h"
#include "mp3_header.h"
#include "mpegindx.h"
#include "cursutil.h"
#include <sys/stat.h>
#include "plugin.h"
#include "mpgsleep.h"


#if !defined(_WIN32)

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

int curs_select_stdin_input(int usec)
{
    struct timeval tv;
    fd_set rfd;

    FD_ZERO(&rfd);
    FD_SET(0, &rfd);
    tv.tv_sec  = 0;
    tv.tv_usec = usec;
    return select(1, &rfd, 0, 0, &tv);
}

#else

int curs_select_stdin_input(int usec)
{
    /* Win32 cannot select on stdin; punt */
    return 1;
}

#endif


static int volume = VOLUME_DEFAULT_VALUE;

void curs_set_volume(int vol)
{
    volume = vol;
}


int curs_get_volume(void)
{
    return volume;
}


int curs_interactive_play_cb(void *data, long sec, long msec)
{
    /* found */
    int inch;
    play_cb_struct *ctx = (play_cb_struct *) data;
    mpegfio_iocallbacks *ttyio = ctx->ttyio;
    int  go = 1;
    long last_sec;
    long last_msec;
    int  volume;
    
    /*
     * 18ms is a trade off.  Spending 18ms/26ms means we are blocking here 70%
     * of the time, which means there *should* be better interactive response
     * to the keyboard, especially to stopping playback.  The main purpose of
     * the change here is to dump all pending audio immediately, by calling
     * mpgedit_play_reset_audio().  The danger of blocking 18ms here is
     * we could starve the sound card of data, resulting in a buffer underflow.
     * This does not seem to occur with mpeg1 layer3 and mpeg2 layer3 data
     * on my slow 200MHz PPro system.
     */
    if (curs_select_stdin_input(18000) && ttyio->getch &&
        (inch = ttyio->getch(NULL)) && inch != -1)
    {
        /*
         * Pause playback if <Enter> is pressed.
         * Resume if <Enter> is pressed again, otherwise stop
         */
        if (inch == '\n') {
            last_sec = mpgedit_play_current_sec(ctx->playctx);
            last_msec = mpgedit_play_current_msec(ctx->playctx);
            mpgedit_play_reset_audio(ctx->playctx);
            mpgedit_play_close(ctx->playctx);
            ctx->playctx = NULL;
            ttyio->printf(ttyio->ctx, 2, 0,
                "Playback paused (press <enter> to continue)\r");
            do {
                mpgedit_usleep(20000);
            } while ((inch = ttyio->getch(NULL)) && inch == -1);

            ctx->playctx = mpgedit_play_init(ctx->filename, 0);
            if (ctx->playctx) {
                volume = curs_get_volume();
                mpgedit_play_volume_init(ctx->playctx, volume, volume);
            }
            mpgedit_play_set_status_callback(ctx->playctx,
                                             curs_interactive_play_cb,
                                             ctx);
            mpgedit_play_seek_time(ctx->playctx, last_sec, last_msec);
        }
        
        if (inch == 'v'){
            volume = curs_volume_decrease(ctx->playctx);
            if (volume != -1) {
                ttyio->printf(ttyio->ctx, 2, 74, "v:%3d", volume);
            }
        }
        else if (inch == 'V'){
            volume = curs_volume_increase(ctx->playctx);
            if (volume != -1) {
                ttyio->printf(ttyio->ctx, 2, 74, "v:%3d", volume);
            }
        }
        else if (inch != '\n') {
            mpgedit_play_reset_audio(ctx->playctx);
            go = 0;
        }

        /*
         * Eat any remaining input characters. This happens when a multi-
         * character key, like an arrow key, is pressed.
         */
        do {
            inch = ttyio->getch(NULL);
        } while (inch && inch != -1);
    }
    if (sec > ctx->esec || (sec == ctx->esec && msec >= ctx->emsec)) {
       go = 0;
    }

    if (sec > ctx->tlast || !go) {
        ctx->tlast = sec;
        if (ttyio && ttyio->printf) {
            ttyio->printf(ttyio->ctx, 2, 0,
                "\rElapsed: %4d:%02d| %5d.%03ds|Frame: %-7d\r",
                   sec/60,
                   sec%60,
                   sec,
                   msec,
                   ctx->frame_count);
        }
    }
    ctx->frame_count++;
    return go;
}


int curs_interactive_play_file(char *filename,
                               char *stime_str,
                               char *dtime_str,
                               mpegfio_iocallbacks *ttyio)
{
    int status = 0;
    mpeg_time stime; /* Start time */
    mpeg_time dtime; /* Play duration */
    long sec;
    long usec;
    long dsec;
    long dusec;
    int go;
    int offset;
    int volume;
    play_cb_struct   play_cb_data;

    memset(&play_cb_data, 0, sizeof(play_cb_data));
    mpeg_time_string2time(stime_str, &stime);
    mpeg_time_gettime(&stime, &sec,  &usec);

    mpeg_time_string2time(dtime_str, &dtime);
    mpeg_time_gettime(&dtime, &dsec, &dusec);


    play_cb_data.playctx = mpgedit_play_init(filename, 0);
    if (play_cb_data.playctx) {
        volume = curs_get_volume();
        mpgedit_play_volume_init(play_cb_data.playctx, volume, volume);
    }
    offset = mpgedit_play_seek_time(play_cb_data.playctx,
                                    sec, usec/1000);
    if (offset == -1) {
        mpgedit_play_reset_audio(play_cb_data.playctx);
        mpgedit_play_close(play_cb_data.playctx);

        /* adam/TBD: This is bogus, need to parameterize these status codes */
        return 5;
    }
    play_cb_data.esec  = sec+dsec;
    play_cb_data.emsec = (usec+dusec)/1000;
    play_cb_data.ttyio = ttyio;
    play_cb_data.filename = filename;
    
    mpgedit_play_set_status_callback(play_cb_data.playctx,
                                     curs_interactive_play_cb, &play_cb_data);
    do {
        go = mpgedit_play_frame(play_cb_data.playctx);
    } while (go);
    mpgedit_play_reset_audio(play_cb_data.playctx);
    mpgedit_play_close(play_cb_data.playctx);
    return status;
}


char *curs_validate_edit_times(editspec_t *edits)
{
    char *errstr = NULL;

    /*
     * Process -e and -f options.
     */
    if (!mpgedit_edit_times_init(edits)) {
        errstr = strdup("-f filename never specified");
#if 0
        fprintf(stderr, "%s: -f filename never specified\n", g_progname);
#endif
        return errstr;
    }

    if (mpgedit_editspec_get_length(edits) > 0) {
        errstr = mpgedit_edit_validate_times(edits, 0, -1);
    }
    else {
        errstr = strdup("no edits present");
    }
    return errstr;
}


int curs_build_indexfile(char *filename,
                         int (*callback)(void *, long, long, long, char *dummy),
                         void *callback_data)

{
    char *indx_filename;
    FILE *fp;
    struct stat sbuf;

    if (stat(filename, &sbuf) == -1) {
        return 1;
    }

    if (!S_ISREG(sbuf.st_mode)) {
        return 1;
    }

    fp = fopen(filename, "rb");
    if (!fp) {
        return 1;
    }
    fclose(fp);

    indx_filename = mpgedit_index_build_filename(filename);
    if (!indx_filename) {
        return 1;
    }

    mp3header_index_create(filename, callback, callback_data, NULL, NULL);

    mpgedit_free(indx_filename);
    return 0;
}


int curs_get_size_from_index(char *filename, long *sec, long *usec)
{
    FILE *indxfp;
    char *indx_filename;
    struct stat sbuf;
    long fsize = 0;

    if (stat(filename, &sbuf) == -1) {
        return 1;
    }

    if (!S_ISREG(sbuf.st_mode)) {
        return 1;
    }


    indx_filename = mpgedit_index_build_filename(filename);
    if (!indx_filename) {
        return 1;
    }
    indxfp = mpgedit_indexfile_open(indx_filename, "rb");
    if (!indxfp) {
        return 1;
    }

    fsize = mp3edit_get_size_from_index(indxfp, sec, usec);
    mpgedit_indexfile_close(indxfp);
    mpgedit_free(indx_filename);
    return !(fsize > 0);
}


int curs_volume_decrease(void *playctx)
{
    int rvol;
    int lvol;
    int volume;
    int sts;

    if (!playctx) {
        return -1;
    }
 
    sts = mpgedit_play_volume_get(playctx, &lvol, &rvol);
    if (sts == -1) {
        return -1;
    }
    volume = (lvol + rvol) / 2;

    volume -= 2;
    volume = (volume < 0) ? 0 : volume;
    mpgedit_play_volume_set(playctx, volume, volume);
    curs_set_volume(volume);

    return volume;
}


int curs_volume_increase(void *playctx)
{
    int rvol;
    int lvol;
    int volume;
    int sts;

    if (!playctx) {
        return -1;
    }
 
    sts = mpgedit_play_volume_get(playctx, &lvol, &rvol);
    if (sts == -1) {
        return -1;
    }
    volume = (lvol + rvol) / 2;

    volume += 2;
    volume = (volume > 100) ? 100 : volume;
    mpgedit_play_volume_set(playctx, volume, volume);
    curs_set_volume(volume);

    return volume;
}

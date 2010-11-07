/*
 * Simple mp3 player.  This is a test of the mpgedit_play API.
 */
#include <stdio.h>
#include <stdlib.h>
#include "playif.h"
#include "editif.h"

#define MP3_TIME_INFINITE 1000000


typedef struct _display_play_stats_t
{
    char *file;
    long  esec;
    long  emsec;
} display_play_stats_t;


/*
 * Callback to display current file play time
 */
int cb_display_play_stats(void *data, long sec, long msec)
{
    display_play_stats_t *ctx = (display_play_stats_t  *) data;
    int go = 1;

    printf("File %s - Elapsed time: %03ld:%02ld.%03ld\r", 
           ctx->file, sec/60, sec%60, msec);
    fflush(stdout);
    if (sec > ctx->esec || (sec == ctx->esec && msec >= ctx->emsec)) {
        go = 0;
    }
    return go;
}


static char *parse_time(char *str, long *rsec, long *rmsec)
{
    char *cp = str;
    int sec = 0, msec = 0;

    *rsec = *rmsec = 0;
    cp    = str;

    if (*cp == '-') {
        return cp;
    }
    sec = strtol(cp, &cp, 10);
    if (*cp == ':') {
        sec *= 60;
        cp++;
        sec += strtol(cp, &cp, 10);
    }
    if (*cp == '.') {
        cp++;
        msec = strtol(cp, &cp, 10);
    }
    *rsec  = sec;
    *rmsec = msec;
    return cp;
}


int main(int argc, char *argv[])
{
    int  go;
    void *ctx;
    char *file;
    char *seek_time;
    long sec  = 0;
    long msec = 0;
    char *cp;
    display_play_stats_t play_ctx;

    if (argc == 1) {
        fprintf(stderr, "usage: %s [mm:ss.mmm] mp3_file\n", argv[0]);
        return 1;
    }
    file = argv[1];

    memset(&play_ctx, 0, sizeof(play_ctx));

    /*
     * Pickup and parse file start time when present.
     */
    if (argc > 2) {
        seek_time = argv[1];
        file      = argv[2];
        cp = parse_time(seek_time, &sec, &msec);
        if (cp[0] == '-' && cp[1]) {
            cp = parse_time(cp+1, &play_ctx.esec, &play_ctx.emsec);
        }
        else {
            play_ctx.esec = MP3_TIME_INFINITE;
        }
    }
    else {
        play_ctx.esec = MP3_TIME_INFINITE;
    }
    play_ctx.file = file;

    /*
     * Index file before playing.  mpgedit_play_seek_time() will not 
     * work without the index file present.
     */
    ctx = mpgedit_edit_index_init(file);
    while (!mpgedit_edit_index(ctx));
    mpgedit_edit_index_free(ctx);

    /*
     * Initialize playback context
     */
    ctx = mpgedit_play_init(file, 0);
    if (!ctx) {
        return 1;
    }

    /*
     * Install play time display callback
     */
    mpgedit_play_set_status_callback(ctx, cb_display_play_stats, &play_ctx);

    /*
     * Seek to start time offset.
     */
    mpgedit_play_seek_time(ctx, sec, msec);

    /*
     * Play file.
     */
    do {
        go = mpgedit_play_frame(ctx);
        /*
         * Insert your code to execute after each second of playback here
         */
    } while (go);
    mpgedit_play_close(ctx);

    return 0;
}

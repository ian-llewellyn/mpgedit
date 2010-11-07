/*
 * File segmentation using minimum amplitude functions.
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
 *
 * RCSID "$Id: segment.c,v 1.3 2004/07/19 15:15:09 number6 Exp $"
 */

#include "portability.h"
#include "mpegindx.h"
#include "xing_header.h"
#include <stdio.h>
#include "pcmlevel.h"
#include "mp3time.h"
#include "segment.h"


/* Complete structure definition for incomplete public type */
struct _mpgedit_pcmlevel_t
{
    int   stereo;
    int   silence_decibels;
    int   silence_threshold;
    int   silence_inflection_threshold;
    int   silence_hysteresis;
    int   silence_hysteresis_set;
    int   silence_repeat;
    int   minimum_inflection_time;
    int   minimum_time;
    int   silence_cnt;
    int   silence_print;

    long  silence_ssec;
    long  silence_smsec;
    long  silence_esec;
    long  silence_emsec;
    int   verbose;
#if 0
    FILE  *pcmfp;
#endif
};


typedef struct _deltaentry {
    int delta;
    long sec;
    long msec;
} deltaentry;


typedef struct _deltalist {
    int indx;
    int max;
    deltaentry *delta;
} deltalist;


deltalist *deltalist_alloc(int num)
{
    deltalist *list;

    list = (deltalist *) calloc(1, sizeof(deltalist));
    if (!list) {
        return NULL;
    }
    list->delta = (deltaentry *) calloc(num, sizeof(deltaentry));
    if (!list->delta) {
        free(list);
        return NULL;
    }
    list->max = num;
    return list;
}


typedef struct _pcmentry_t {
    int  pcmlevel;
    long sec;
    long msec;
    long fpos;
    struct _pcmentry_t *next;
} pcmentry_t;


typedef struct _pcmlist_t {
    pcmentry_t *head;
    pcmentry_t *cursor;
    pcmentry_t *tail;
    int        count;
} pcmlist_t;




void deltalist_insert(deltalist *list, int delta, long sec, long msec)
{
    if (list->indx == list->max) {
        memmove(list->delta, &list->delta[1], sizeof(deltaentry)*(list->max-1));
        list->delta[list->max-1].delta = delta;
        list->delta[list->max-1].sec   = sec;
        list->delta[list->max-1].msec  = msec;
    }
    else {
        list->delta[list->indx].delta = delta;
        list->delta[list->indx].sec   = sec;
        list->delta[list->indx].msec  = msec;
        list->indx++;
    }
}


pcmentry_t *pcmentry_list_insert(pcmlist_t *list, int level,
                                 long sec, long msec, long fpos)
{
    pcmentry_t *entry;

    entry = calloc(1, sizeof(pcmentry_t));
    if (!entry) {
        return NULL;
    }
    if (!list->head) {
        list->head = entry;
        list->tail = entry;
    }
    else {
        list->tail->next = entry;
        list->tail       = entry;
    }
    list->count++;
    entry->pcmlevel = level;
    entry->sec      = sec;
    entry->msec     = msec;
    entry->fpos     = fpos;
    return entry;
}


int pcmentry_list_count(pcmlist_t *list)
{
    return list->count;
}


pcmentry_t *pcmentry_list_first(pcmlist_t *list)
{
    if (!list || !list->head) {
        return NULL;
    }
    list->cursor = list->head;
    return list->head;
}


pcmentry_t *pcmentry_list_next(pcmlist_t *list)
{
    if (!list || !list->cursor || !list->cursor->next) {
        return NULL;
    }
    list->cursor = list->cursor->next;
    return list->cursor;
}


void pcmentry_list_free(pcmlist_t *list)
{
    pcmentry_t *node;
    pcmentry_t *next;

    if (!list || !list->head) {
        return;
    }

    node = list->head;
    do {
        next = node->next;
        free(node);
        node = next;
    } while (next);
    list->head = list->tail = NULL;
}


int deltalist_slopes(deltalist *list, deltaentry *slopes, int *ret_pcmmin)
{
    int i;
    int j;
    int n;
    int pcmmin = 100000;

    /* Number of samples contained in the slope computation */
    int lookahead = 5;

    /* Don't even look at a list not completely full */
    if (list->indx != list->max) {
        return 0;
    }

    for (i=0; i<list->max; i++) {
        if (list->delta[i].delta < pcmmin) {
            pcmmin = list->delta[i].delta;
        }
    }

    n = 0;
    for (i=0, j=lookahead; j<list->max; i++, j++) {
        slopes[n].delta = list->delta[j].delta - list->delta[i].delta;
        slopes[n].sec   = list->delta[j].sec;
        slopes[n].msec  = list->delta[j].msec;
        n++;
    }
    if (ret_pcmmin) {
        *ret_pcmmin = pcmmin;
    }
    return n;
}


/*
 * This function is a bit out of control.  It is trying to do too much in 
 * one place.  The functionality used here is just the minimum threshold 
 * detection.  The amplitude inflection point detection is not used.  That
 * functionality will be refactored out of this function, and done elsewhere.
 *
 * The current plan for segment boundary detection is to do a first order
 * analysis, the current minimum threshold detection.  Given that is the 
 * most reliable detection.  The second order analysis will perform
 * amplitude inflection detection on the segments bracked by the boundaries
 * detected here.
 *
 * Given a mp3 levels file (.lvl file) generated by mpgedit, analyze
 * each frame's average amplitude for a sequence of frames that are below
 * the minimum amplitude intensity.  Add that time to the segments list.
 * The mpgedit_pcmlevel_t parameter allows the caller to pass configurable
 * parameters that control the segment boundary search.
 * 
 * Currently these parameters are viewed as most important to the caller:
 *
 *  verbose:          Display each frame PCM level
 *  silence_decibels: Specify the decibel level below the PCM average
 *                    that defines the "silence threshold".  The default
 *                    is 30db down when not specified.
 *  minimum_time:     Minimum number of seconds that must pass since the
 *                    last boundary was detected before the next boundary
 *                    will be detected.
 *  silence_repeat    The number of consecutive frames below the
 *                    silence threshold that must occur before a segment
 *                    boundary is detected.
 *  minimum_inflection_time:
 *                    Similar to minimum_time.  The number of seconds that must
 *                    pass since the last inflection point boundary detection
 *                    before the next can be detected.
 */
static int detect_segments(char *levels_file,
                           mpgedit_pcmlevel_t *level,
                           pcmlist_t *segments)
{
    int ver;
    int pcmbits;
    int secbits;
    int msecbits;
    mpgedit_pcmfile_t *pcmfp;
    int avg;
    int pcmmax;
    long sec, msec;
    deltalist  *deltas;
    deltaentry *delta_slopes;
    int lastsec, lastmsec;
    int pcmmax_last = 0;
    pcmlist_t pcmlist;
    int file_offset;
    int go;
    pcmentry_t *entry;
    pcmentry_t *entry_min  = NULL;
    pcmentry_t *entry_prev = NULL;
    int pcmlist_min;

    levels_file = mpgedit_index_build_filename_2(levels_file, ".lvl");
    if (!levels_file) {
        return 2;
    }
    pcmfp = mpgedit_pcmlevel_open(levels_file, "rb");
    free(levels_file);
    if (!pcmfp) {
        perror("fopen");
        return -1;
    }
    mpgedit_pcmlevel_read_header(pcmfp, &ver, &pcmbits, &secbits, &msecbits);
    if (ver != 1 || pcmbits != 16 || secbits != 22 || msecbits!= 10) {
#ifdef D_S_DEBUG
        printf("ERROR: Invalid levels file header; unknown version %d\n", ver);
#endif
        return 1;
    }
    mpgedit_pcmlevel_read_average(pcmfp, &avg, NULL, NULL);
#ifdef D_S_DEBUG
    printf("average volume: %d\n", avg);
#endif

    memset(&pcmlist, 0, sizeof(pcmlist));
    deltas       = deltalist_alloc(20);
    delta_slopes = (deltaentry *) calloc(20, sizeof(deltaentry));
    lastsec      = lastmsec = -1; 

    pcmentry_list_free(segments);

    /*
     * Right shift 5 is about 30db difference in dynamic range.
     * Right shift 4 is about 24db difference in dynamic range.
     * 6db/bit you know [db = 20 * log(2^bits)].
     */
    level->silence_threshold            = avg >> level->silence_decibels;
    level->silence_inflection_threshold = level->silence_threshold * 2;
    level->silence_hysteresis           = level->silence_threshold /
                                              HYSTERESIS_RATIO;

#ifdef D_S_DEBUG
    printf("silence volume: %d\n", level->silence_threshold);
    printf("silence hysteresis: %d\n", level->silence_hysteresis);
#endif

    file_offset = mpgedit_pcmlevel_tell(pcmfp);
    go = mpgedit_pcmlevel_read_entry(pcmfp, &pcmmax, &sec, &msec);
    while (go) {
        /*
         * Zero volume frame samples must be due to 
         * decoding errors from occasional inability to rewind
         * bitstream.  Work around by using last non-zero frame value.
         */
        if (pcmmax == 0) {
            pcmmax = pcmmax_last;
        }
        else {
            pcmmax_last = pcmmax;
        }
#if 0 /* start inflection point detection */
        deltalist_insert(deltas, pcmmax, sec, msec);
        slopes = deltalist_slopes(deltas, delta_slopes, &slopes_min);
        if (save_neg_slope_cnt > 0) {
            save_neg_slope_cnt--;
        }
        else if (slopes) {
            neg_slope_cnt = 0;
            pos_slope_cnt = 0;
            for (i=0; i<slopes; i++) {
                if (delta_slopes[i].delta < 0) {
                    /*
                     * Negative average slope found
                     */
                    neg_slope_cnt++;
                    pos_slope_cnt = 0;
                }
                else {
                    /*
                     * Positive average slope found
                     */
                    if ((save_neg_slope_cnt == 0) &&
                         (slopes_min <
                              level->silence_inflection_threshold) &&
                        (delta_slopes[i].sec >
                            (lastsec + level->minimum_inflection_time)) &&
                        (neg_slope_cnt > NEGATIVE_SLOPE_REPEAT))
                    {
                        found_poi = 1;
                        pcmlevel_poi.pcmlevel = pcmmax;
                        pcmlevel_poi.sec = sec;
                        pcmlevel_poi.msec = msec;
                        pcmlevel_poi.fpos = file_offset;

                        entry_poi.pcmlevel = slopes_min;
                        entry_poi.sec      = delta_slopes[i].sec;
                        entry_poi.msec     = delta_slopes[i].msec;
                        entry_poi.fpos     = file_offset;
#ifdef D_S_DEBUG
                        printf("\n\n  ===  Found point of interest "
                                "%6ld:%02ld.%03ld slope=%d  ===\n", 
                                delta_slopes[i].sec/60, 
                                delta_slopes[i].sec%60, 
                                delta_slopes[i].msec,
                                delta_slopes[i-1].delta);
#endif
                                save_neg_slope_cnt = neg_slope_cnt;
#if 0 /* add */
                        sec  = delta_slopes[i].sec;
                        msec = delta_slopes[i].msec;
                        pcmentry_list_insert(segments, pcmmax, sec,
                                             msec, file_offset);
                        lastsec  = sec;
                        lastmsec = msec;
#endif /* add */
                    }
                    neg_slope_cnt = 0;
                    pos_slope_cnt++;
                }
            }
        }
#endif /* end inflection point detection */

#if 0
if (sec == 55*60 + 58) {
printf("breakpoint\n");
}
#endif
        if ((level->silence_hysteresis_set == 0 &&
             pcmmax < level->silence_threshold) ||
             pcmmax < (level->silence_threshold +
                       level->silence_hysteresis_set))
        {
            level->silence_cnt++;
            if (level->silence_ssec == 0 && level->silence_smsec == 0) {
                pcmentry_list_free(&pcmlist);
                level->silence_hysteresis_set =
                    pcmmax + level->silence_hysteresis;
#ifdef D_S_DEBUG
                printf("  --> threshold=%d:%d/pcmavg=%6d (%6ld:%02ld.%03ld) <--\n",
                    level->silence_threshold,
                    level->silence_hysteresis_set,
                    pcmmax, sec/60, sec%60, msec);
#endif
                level->silence_ssec  = sec;
                level->silence_smsec = msec;
            }
            else {
                level->silence_esec  = sec;
                level->silence_emsec = msec;
            }

            if (level->silence_cnt >= level->silence_repeat) {
                if (lastsec == -1 || sec > (lastsec+level->minimum_time)) {
                    level->silence_print = 0;
                    pcmentry_list_insert(&pcmlist, pcmmax, sec, msec, file_offset);
                }
            }
        }
        else if (level->silence_hysteresis_set > 0 &&
                  pcmmax >= level->silence_hysteresis_set)
        {
#ifdef D_S_DEBUG
printf("exceeded silence threshold: %d\n", pcmmax);
#endif
            if (!level->silence_print) {
#ifdef D_S_DEBUG
                printf("threshold=%d:%d/pcmavg=%6d (%6ld:%02ld.%03ld => %6ld:%02ld.%03ld)\n",
                    level->silence_threshold,
                    level->silence_hysteresis_set,
                    pcmmax,
                    level->silence_ssec/60,
                    level->silence_ssec%60,
                    level->silence_smsec,
                    level->silence_esec/60,
                    level->silence_esec%60,
                    level->silence_emsec);
                printf("================================\n");
#endif


                /*
                 * Do better than just an average...  Look at the
                 * volume decay and attack to figure if an average
                 * or value closer to start of next track is better.
                 */
                if ((level->silence_esec - level->silence_ssec) > 2) {
                    sec  = level->silence_esec - 2;
                    msec = level->silence_emsec;
                }
                else {
                    avg  = (level->silence_esec - level->silence_ssec) * 1000;
                    avg += level->silence_emsec - level->silence_smsec;
                    avg /= 2;
                    
                    sec  = level->silence_esec  - avg / 1000;
                    msec = level->silence_emsec - avg % 1000;
                    if (msec < 0) {
                        msec += 1000;
                        sec--;
                        if (sec < 0) {
                            sec  = 0;
                            msec = 0;
                        }
                    }
                }
                pcmlist_min = 100000;
                entry = pcmentry_list_first(&pcmlist);
                if (entry) {
                    /*
                     * Search for minimum amplitude sample between
                     * boundaries.  Use that sample.
                     */
                    while (entry) {
                        if (entry->pcmlevel < pcmlist_min) {
                            pcmlist_min = entry->pcmlevel;
                            entry_min = entry;
                        }
#ifdef D_S_DEBUG
                        printf(" --> pcmlist %d %ld:%02ld.%03ld\n",
                               entry->pcmlevel, entry->sec/60, 
                               entry->sec%60, entry->msec);
#endif
                        entry_prev = entry;
                        entry = pcmentry_list_next(&pcmlist);
                    }
                    
                    if (!entry || entry_min != entry_prev) {
                        pcmmax      = entry_min->pcmlevel;
                        sec         = entry_min->sec;
                        msec        = entry_min->msec;
                        file_offset = entry_min->fpos;
                    }
                }
#if 0
                else 
                if (found_poi) {
                    pcmmax      = entry_poi.pcmlevel;
                    sec         = entry_poi.sec;
                    msec        = entry_poi.msec;
                    file_offset = entry_poi.fpos;
#ifdef D_S_DEBUG
                    printf(" --> pcmlevel_poi=%d %ld:%02ld.%03ld\n", 
                    pcmlevel_poi.pcmlevel, pcmlevel_poi.sec/60, pcmlevel_poi.sec%60,
                    pcmlevel_poi.msec);
#endif
                }
                found_poi = 0;
#endif
                pcmentry_list_insert(segments, pcmmax, sec, msec, file_offset);
                lastsec  = sec;
                lastmsec = msec;
                level->silence_print = 1;
            }
            level->silence_cnt   = 0;
            level->silence_ssec  = 0;
            level->silence_smsec = 0;
            level->silence_hysteresis_set = 0;
        }
        if (level->verbose) {
            printf("%6d %5ld:%02ld.%03ld\n", pcmmax, sec/60, sec%60, msec);
        }
        go = mpgedit_pcmlevel_read_entry(pcmfp, &pcmmax, &sec, &msec);
        file_offset += 6;
    }
    mpgedit_pcmlevel_close(pcmfp);
    return 0;
}


static int segments_to_mpeg_time(pcmlist_t *segments,
                          mpeg_time **rtarray, int *rtarraylen)
{
    int        len;
    mpeg_time  *tarray;
    pcmentry_t *pcmentry;
    int        i;

    if (!segments || !rtarray || !rtarraylen) {
        return -1;
    }

    len = pcmentry_list_count(segments);
    if (len <= 0) {
        return -1;
    }
    tarray = (mpeg_time *) calloc(len, sizeof(mpeg_time)); 
    if (!tarray) {
        return -1;
    }
    i = 0;
    pcmentry = pcmentry_list_first(segments);
    while (pcmentry) {
        tarray[i].units = pcmentry->sec;
        tarray[i].usec  = pcmentry->msec * 1000;
        i++;
        pcmentry = pcmentry_list_next(segments);
    }
    *rtarray    = tarray;
    *rtarraylen = len;
    return 0;
}


mpgedit_pcmlevel_t *mpgedit_pcmlevel_new(mpgedit_pcmlevel_t *template)
{
    mpgedit_pcmlevel_t *newlvl;
    newlvl = (mpgedit_pcmlevel_t *) calloc(1, sizeof(mpgedit_pcmlevel_t));
    if (newlvl && template) {
        *newlvl = *template;
    }
    return newlvl;
}


void mpgedit_pcmlevel_free(mpgedit_pcmlevel_t *pcm)
{
    if (pcm) {
        free(pcm);
    }
}





int mpgedit_segment_find(char *name, mpgedit_pcmlevel_t *levelconf,
                         mpeg_time **segments, int *segmentslen)
{
    pcmlist_t  segment_list;
    mpgedit_pcmlevel_t *deflevelconf;
    mpgedit_pcmlevel_t deflevelconf_data;

    memset(&segment_list, 0, sizeof(segment_list));

    if (!levelconf) {
        memset(&deflevelconf_data, 0, sizeof(deflevelconf_data));
        deflevelconf                      = &deflevelconf_data;
        deflevelconf->minimum_inflection_time = MINIMUM_INFLECTION_TIME;
        deflevelconf->silence_decibels    = SILENCE_30DB;
        deflevelconf->minimum_time        = MINIMUM_TIME;
        deflevelconf->silence_repeat      = SILENCE_REPEAT;
    }
    else {
        deflevelconf = levelconf;
    }
    detect_segments(name, deflevelconf, &segment_list);
    return segments_to_mpeg_time(&segment_list, segments, segmentslen);
}


void mpgedit_pcmlevel_init_defaults(mpgedit_pcmlevel_t *pcm)
{
    if (pcm) {
        pcm->minimum_inflection_time = MINIMUM_INFLECTION_TIME;
        pcm->silence_decibels        = SILENCE_30DB;
        pcm->minimum_time            = MINIMUM_TIME;
        pcm->silence_repeat          = SILENCE_REPEAT;
    }
}


void mpgedit_pcmlevel_set_inflection_time(mpgedit_pcmlevel_t *pcm,
                                          int inftime)
{
    if (pcm) {
        pcm->minimum_inflection_time = inftime;
    }
}


void mpgedit_pcmlevel_set_silence_decibels(mpgedit_pcmlevel_t *pcm, 
                                           int decibels)
{
    if (pcm) {
        pcm->silence_decibels = decibels;
    }
}


void mpgedit_pcmlevel_set_minimum_time(mpgedit_pcmlevel_t *pcm, 
                                       int mintime)
{
    if (pcm) {
        pcm->minimum_time = mintime;
    }
}


void mpgedit_pcmlevel_set_silence_repeat(mpgedit_pcmlevel_t *pcm, 
                                         int repeat)
{
    if (pcm) {
        pcm->silence_repeat = repeat;
    }
}

int mpgedit_pcmlevel_get_inflection_time(mpgedit_pcmlevel_t *pcm)
{
    if (pcm) {
        return pcm->minimum_inflection_time;
    }
    return -1;
}


int mpgedit_pcmlevel_get_silence_decibels(mpgedit_pcmlevel_t *pcm)
{
    if (pcm) {
        return pcm->silence_decibels;
    }
    return -1;
}


int mpgedit_pcmlevel_get_minimum_time(mpgedit_pcmlevel_t *pcm)
{
    if (pcm) {
        return pcm->minimum_time;
    }
    return -1;
}


int mpgedit_pcmlevel_get_silence_repeat(mpgedit_pcmlevel_t *pcm)
{
    if (pcm) {
        return pcm->silence_repeat;
    }
    return -1;
}

#ifndef _SEGMENT_H
#define _SEGMENT_H

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
 * RCSID "$Id: segment.h,v 1.2 2004/07/19 14:44:04 number6 Exp $"
 */

#include "portability.h"
#include <stdio.h>

#define MINIMUM_TIME            90
#define MINIMUM_INFLECTION_TIME 90
#define SILENCE_THRESHOLD       900
#define SILENCE_REPEAT          3
#define SILENCE_6DB             1
#define SILENCE_12DB            2
#define SILENCE_18DB            3
#define SILENCE_24DB            4
#define SILENCE_30DB            5
#define SILENCE_36DB            6
#define HYSTERESIS_RATIO        10
#define NEGATIVE_SLOPE_REPEAT   8

typedef struct _mpgedit_pcmlevel_t mpgedit_pcmlevel_t;

#if 0 /* incomplete definition completed in segment.c and pcmlevel.c */
typedef struct _mpgedit_pcmlevel_t
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
    FILE  *pcmfp;
} mpgedit_pcmlevel_t;
#endif

_DSOEXPORT mpgedit_pcmlevel_t * _CDECL 
    mpgedit_pcmlevel_new(mpgedit_pcmlevel_t *template);

_DSOEXPORT void                 _CDECL 
    mpgedit_pcmlevel_free(mpgedit_pcmlevel_t *pcm);

_DSOEXPORT int                  _CDECL 
    mpgedit_segment_find(char *name,
                         mpgedit_pcmlevel_t *levelconf,
                         mpeg_time **segments,
                         int *segmentslen);

_DSOEXPORT void                 _CDECL 
    mpgedit_pcmlevel_init_defaults(mpgedit_pcmlevel_t *pcm);

_DSOEXPORT void                 _CDECL 
    mpgedit_pcmlevel_set_inflection_time(mpgedit_pcmlevel_t *pcm, int inftime);

_DSOEXPORT void                 _CDECL 
    mpgedit_pcmlevel_set_silence_decibels(mpgedit_pcmlevel_t *pcm,
                                          int decibels);
_DSOEXPORT void                 _CDECL 
    mpgedit_pcmlevel_set_minimum_time(mpgedit_pcmlevel_t *pcm, int mintime);

_DSOEXPORT void                 _CDECL 
    mpgedit_pcmlevel_set_silence_repeat(mpgedit_pcmlevel_t *pcm, int repeat);

_DSOEXPORT int                 _CDECL 
    mpgedit_pcmlevel_get_inflection_time(mpgedit_pcmlevel_t *pcm);

_DSOEXPORT int                 _CDECL 
    mpgedit_pcmlevel_get_silence_decibels(mpgedit_pcmlevel_t *pcm);

_DSOEXPORT int                 _CDECL 
    mpgedit_pcmlevel_get_minimum_time(mpgedit_pcmlevel_t *pcm);

_DSOEXPORT int                 _CDECL 
    mpgedit_pcmlevel_get_silence_repeat(mpgedit_pcmlevel_t *pcm);

#endif

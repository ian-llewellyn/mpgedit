/*
 * PCM data volume analysis
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

#include "wav16.h"
#include <limits.h>

#define SWAP_16(v) { \
    unsigned short t = (unsigned short) (v); \
    unsigned char *o = (unsigned char *) &(v); \
    unsigned char *i = (unsigned char *) &t; \
    o[0] = i[1]; \
    o[1] = i[0]; \
}


/* These are backwards from what you would think they should be. */
#define MIN_LEVEL_INIT  SHRT_MAX
#define MAX_LEVEL_INIT  SHRT_MIN


/*
 * Return array 'samples' containing the volumes for each
 * sample in input PCM data array 'buf'.
 */
void compute_wav16_samples(const unsigned char *buf, int len, int channels,
                           int *samples, int *sampleslen, int bswap)
{
    /* Used for 16 bit samples */
    const short int *left;
    const short int *right;
    short int l;
    short int r;
    int   indx;

    indx  = 0;
    left  = (short int *) buf;
    right = left + 1;
    while (len > 0) {
        l = *left;
        r = *right;
        if (bswap) {
            SWAP_16(l);
            SWAP_16(r);
        }
        samples[indx++] = l;
        len -= sizeof(l);
        left++;

        /* Stereo, pickup the right channel data */
        if (channels == 2) {
            samples[indx++] = r;
            len -= sizeof(r);
            /* 
             * move the left pointer forward again, to skip over 
             * right channel data value.
             */
            left++;
            right = left + 1;
        }
    }
    *sampleslen = indx;
}


/*
 * Return the maximum amplitude for the buffer of 16 bit WAV audio samples.
 */
int wav16_samples_max(const unsigned char *buf, int len, int channels, 
                      int bswap)
{
    /* Used for 16 bit samples */
    const short int *left;
    const short int *right;
    short int l;
    short int r;
    int   lavg;
    int   ravg;

    int  lmin = MIN_LEVEL_INIT;
    int  lmax = MAX_LEVEL_INIT;
    int  rmin = MIN_LEVEL_INIT;
    int  rmax = MAX_LEVEL_INIT;

    if (len == 0) {
        return 0;
    }

    left  = (short int *) buf;
    right = left + 1;
    while (len > 0) {
        l = *left;
        r = *right;
        if (bswap) {
            SWAP_16(l);
            SWAP_16(r);
        }
        /*
         * Save left channel max/min volume
         */
        if (l > lmax) {
            lmax = l;
        }
        else if (l < lmin) {
            lmin = l;
        }
        len -= sizeof(l);
        left++;

        /* Stereo, pickup the right channel data */
        if (channels == 2) {
            if (r > rmax) {
                rmax = r;
            }
            else if (r < rmin) {
                rmin = r;
            }
            len -= sizeof(r);
            /* 
             * move the left pointer forward again, to skip over 
             * right channel data value.
             */
            left++;
            right = left + 1;
        }
    }
    lavg = lmax - lmin;
    if (channels == 2) {
        ravg = rmax - rmin;
        return lavg > ravg ? lavg : ravg;
    }
    return lavg;
}

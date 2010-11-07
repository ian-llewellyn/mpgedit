/*
 * Microsecond resolution time accounting header
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

/*
 * SccsId[] = "$Id: mp3time.h,v 1.10 2009/02/04 07:45:49 number6 Exp $"
 */

#ifndef _MP3TIME_H
#define _MP3TIME_H

#include <string.h>
#include "header.h"
#include "portability.h"

#define MP3_TIME_INFINITE 1000000

typedef struct _mpeg_time {
    long units;
    long usec;        
} mpeg_time;



_DSOEXPORT void _CDECL mpeg_time_clear(mpeg_time *tval);
void mpeg_time_frame_increment(mpeg_time *tval, mpeg_header_data *header);
long mpeg_time_2seconds(mpeg_time *tval);
_DSOEXPORT void _CDECL mpeg_time_init(mpeg_time *tval, int sec, int usec);
long mpeg_time_get_seek_offset(FILE *indxfp,
                               mpeg_time *stime,
                               mpeg_time *seektime);

_DSOEXPORT void _CDECL mpeg_time_gettime(mpeg_time *tval, 
                                         long *sec, long *usec);
_DSOEXPORT long _CDECL mpeg_time_getsec(mpeg_time *tval);
_DSOEXPORT long _CDECL mpeg_time_getusec(mpeg_time *tval);
long mpeg_time_seek_starttime(FILE      *fp,
                              FILE      *indxfp,
                              mpeg_time *stime,
                              mpeg_time *mpegtval,
                              int       has_xing);

_DSOEXPORT mpeg_time _CDECL mpeg_time_compute_delta(long ssec, long susec,
                                                    long esec, long eusec);

_DSOEXPORT void _CDECL mpeg_time_string2time(char *s, mpeg_time *tval);
#endif

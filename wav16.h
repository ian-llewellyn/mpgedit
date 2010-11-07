#ifndef _WAV16_H
#define _WAV16_H
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

#define ISBIGENDIAN(val) \
{ long l = 1; unsigned char *c; c = (unsigned char *) &l; ((val) = (*c==0));}

void compute_wav16_samples(const unsigned char *buf, int len, int channels,
                           int *samples, int *sampleslen, int bswap);
int wav16_samples_max(const unsigned char *buf,
                      int len,
                      int channels,
                      int bswap);

#endif

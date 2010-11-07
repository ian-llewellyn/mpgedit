/*
 * SccsId[] = "$Id: interface.h,v 1.2 2003/10/27 07:06:19 number6 Exp $"
 */

/*
** Copyright (C) 2000 Albert L. Faber
** Modified for use in mpgedit by Adam Bernstein.
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef INTERFACE_H_INCLUDED
#define INTERFACE_H_INCLUDED

#include "common.h"

BOOL InitMP3(PMPSTR mp);
int	 decodeMP3(PMPSTR mp,
                   unsigned char *inmemory,int inmemsize,
                   char *outmemory,int outmemsize,
                   int *done);

/*
 * Same as decodeMP3, but allows previous frame to be passed in,
 * needed by layer3 for backspace bits access to work.
 *
 * New parameters added: 
 *   prev_in  - Frame before the frame being decoded.
 *   prev_len - Byte count of prev_in
 *   bsbytes  - Number of bytes decoder will backspace into prev_in.
 *              Used by mpgedit to determine the number of previous
 *              bits to add to synthesized edit prefix frame.
 * All of these new parameters are optional.
 */
int decodeMP3_9(PMPSTR mp,
                unsigned char *in, int isize,
                unsigned char *prev_in, int prev_isize,
                char *out,
                int osize,int *done,
                int *bsbytes);


void ExitMP3(PMPSTR mp);

/* added remove_buf to support mpglib seeking */
void remove_buf(PMPSTR mp);

#endif

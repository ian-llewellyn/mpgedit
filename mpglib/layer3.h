/*
 * SccsId[] = "$Id: layer3.h,v 1.2 2003/10/27 07:06:19 number6 Exp $"
 */

/*
** Copyright (C) 2000 Albert L. Faber
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

#ifndef LAYER3_H_INCLUDED
#define LAYER3_H_INCLUDED

#include "mpg123.h"

void init_layer3(int);
int  do_layer3_sideinfo( struct frame *fr, struct III_sideinfo *sideinfo);
int  do_layer3( PMPSTR mp,unsigned char *pcm_sample,int *pcm_point,
                struct III_sideinfo *sideinfo);



#endif


/*
 * xmpgedit volume control interface
 *
 * Copyright (C) 2003-2006 Adam Bernstein
 * All rights reserved.
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

#ifndef _VOLUMEAPI_H
#define _VOLUMEAPI_H

#define MIXER_DEVICE "/dev/mixer"

void * mpgedit_volume_open(void);
void   mpgedit_volume_close(void *ctx);
int    mpgedit_volume_get(void *fd, int *lvol, int *rvol);
void   mpgedit_volume_set(void *fd, int lvol, int rvol);


#endif

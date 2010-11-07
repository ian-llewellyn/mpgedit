/*
 * mpgedit curses interface header
 *
 * Copyright (C) 2001-2004 Adam Bernstein. All Rights Reserved.
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
 * SccsId[] = "$Id: mpegcurses.h,v 1.9 2005/11/19 18:30:39 number6 Exp $"
 */

#ifndef _MPEG_CURSES_H
#define _MPEG_CURSES_H

#include "mp3_header.h" 
#include "editif.h" /* for editspec typedef */
#include "segment.h"

int curs_play(cmdflags   *argvflags, 
              editspec_t **ret_edits,
              mpgedit_pcmlevel_t *levelconf);
#endif


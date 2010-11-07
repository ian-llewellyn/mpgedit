/*
 * mpgedit curses interface stubs for non-curses build
 *
 * Copyright (C) 2001-2006 Adam Bernstein. All Rights Reserved.
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

#ifndef lint
static char SccsId[] = "$Id: cursesstub.c,v 1.8 2005/11/19 18:30:38 number6 Exp $";
#endif

#include <stdio.h>
#include "mp3time.h"
#include "mp3_header.h"
#include "mpegcurses.h"
#include "pcmlevel.h"


int curs_play(cmdflags   *argvflags,
              editspec_t **ret_edits,
              mpgedit_pcmlevel_t *levelconf)
{
    fprintf(stderr, "Error: no curses support in this build\n\n");
    usage(0);
    return 1;
}

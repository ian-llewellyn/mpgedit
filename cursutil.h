/*
 * mpgedit curses helper utility functions header.
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

#ifndef _CURSUTIL_H_
#define _CURSUTIL_H_

int curs_interactive_play_cb(void *data, long sec, long msec);

int curs_interactive_play_file(char *filename,
                               char *stime_str,
                               char *dtime_str,
                               mpegfio_iocallbacks *ttyio);
char *curs_validate_edit_times(editspec_t *edits);


int curs_build_indexfile(char *filename,
                         int (*callback)(void *, long, long, long, char *),
                         void *callback_data);

int curs_get_size_from_index(char *filename, long *sec, long *usec);
int curs_volume_increase(void *playctx);
int curs_volume_decrease(void *playctx);

void curs_set_volume(int vol);
int  curs_get_volume(void);
#endif

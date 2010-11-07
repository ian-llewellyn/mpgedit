/*
 * XING header editing abstraction
 *
 * Copyright (C) 2009 Adam Bernstein
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
#include <stdio.h>
#include "mpegstat.h"

#ifndef _XINGEDIT_H
#define _XINGEDIT_H


typedef enum MPGEDIT_XING_MODIFY_ACTION {
  MPGEDIT_XING_MODIFY_SET,
  MPGEDIT_XING_MODIFY_INCR,
} MPGEDIT_XING_MODIFY_ACTION_en;

/*
 * Modify Xing header frame/byte value for the open file referenced by fp.
 * The values are either set or incremented, depending on the value of action.
 */
int mpgedit_xing_modify_fp(FILE *fp, 
                           long frames, 
                           long bytes, 
                           MPGEDIT_XING_MODIFY_ACTION_en action);

/*
 * Modify Xing header frame/byte value for the file referenced by name.
 * The values are either set or incremented, depending on the value of action.
 */
int mpgedit_xing_modify_file(char *name, 
                             long frames, 
                             long bytes, 
                             MPGEDIT_XING_MODIFY_ACTION_en action);

/*
 * Modify Xing header using values in stats structure for the open 
 * file referenced by fp.
 * The values are either set or incremented, depending on the value of action.
 */
int mpgedit_xing_stats_modify_fp(
        FILE *fp,
        mpeg_file_stats *stats,
        MPGEDIT_XING_MODIFY_ACTION_en action);

/*
 * Modify Xing header using values in stats structure for the file 
 * referenced by name.
 * The values are either set or incremented, depending on the value of action.
 */
int mpgedit_xing_stats_modify_file(
        char *name, 
        mpeg_file_stats *stats,
        MPGEDIT_XING_MODIFY_ACTION_en action);
#endif

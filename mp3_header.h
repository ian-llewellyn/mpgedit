/*
 * mpgedit application code header file
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

/*
 * SccsId[] = "$Id: mp3_header.h,v 1.10 2005/11/19 18:30:39 number6 Exp $"
 */

#ifndef _MP3_HEADER_H
#define _MP3_HEADER_H


#include "mp3time.h"
#include "mpegstat.h"
#include "editif.h"


typedef struct _mpegfio_iocallbacks
{
    void *ctx;
    void (*printf)(void *, int, int, const char *, ...);
    int  (*getch)(void *);
    void *mpctx;
    int  cb_called;
} mpegfio_iocallbacks;


typedef struct _play_cb_struct
{
    void  *playctx;
    char  *filename;
    mpegfio_iocallbacks *ttyio;
    long  esec;
    long  emsec;
    long  tlast;
    long  frame_count;
} play_cb_struct;


typedef struct _cmdflags
{
    editspec_t *edits;
    int      edits_cnt;
    char *out_filename;
    int  out_fileappend;
    int O_flag;
    int v_flag;
    int p_flag;
    int e_flag;
    int E_flag;
    int X_flag;
    char *X_args;
    int X_val;
    int c_flag;
    int s_flag;
    int has_xing;
    int L_flag;
    int l_flag;
    int vol_left;
    int vol_right;
    int d_flag;
    int d_val;
    int D_flag;
    int Ds_flag;
    int a_flag;
    int I_flag;
    char *l_value;
} cmdflags;


void usage(int longhelp);

int mp3header_index_create(char *filename,
                           int (*callback)(void *,   /* callback_data */
                                           long,     /* sec */
                                           long,     /* msec */
                                           long,     /* frame_count */
                                           char *),  /* index filename */
                           void *callback_data,
                           mpeg_file_stats *rstats,
                           mpeg_time *rtime);

void _mpgedit_decode_file(char *fname,
                          long ssec, long susec,
                          long esec, long eusec,
                          cmdflags *argvflags,
                          mpegfio_iocallbacks *ttyio);

#endif

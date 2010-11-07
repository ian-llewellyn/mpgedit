/*
 * mpgedit editing interface
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

#ifndef _EDITIF_H_
#define _EDITIF_H_

#include "mpegstat.h"

#define MPGEDIT_FLAGS_APPEND        1

#define MPGEDIT_FLAGS_XING_DEFAULT  0x0
#define MPGEDIT_FLAGS_XING_OMIT     0x2  /* Prune XING header from output     */
#define MPGEDIT_FLAGS_XING_FIXUP    0x4  /* Update XING header with new stats */
#define MPGEDIT_FLAGS_NO_EDITS      0x8  /* Perform no edits; stats only      */
#define MPGEDIT_FLAGS_EACH_FRAME    0x10 /* Return after editing every frame  */
#define MPGEDIT_FLAGS_MD5SUM_FRAME  0x20 /* Checksum each mp3 frame           */
#define MPGEDIT_FLAGS_GET_STATS     0x40 /* Only populate index stats header  */
#define MPGEDIT_FLAGS_NO_INDEX      0x80 /* Perform edits without .idx file   */
#define MPGEDIT_FLAGS_NO_AUDIODEV   0x100/* mpgedit_init() will succeed even
                                            if fails to open audio device     */

#define MPGEDIT_EDIT_FILES_ERR_OUTFILE_CANT_APPEND 1
#define MPGEDIT_EDIT_FILES_ERR_OUTFILE_EXISTS      2
#define MPGEDIT_EDIT_FILES_ERR_INFILE_ERR          3
#define MPGEDIT_EDIT_FILES_ERR_INDEXFILE_ERR       4
#define MPGEDIT_EDIT_FILES_ERR_OUTFILE_OPEN        5
#define MPGEDIT_EDIT_FILES_ERR_START_TIME          6
#define MPGEDIT_EDIT_FILES_BAD_CONTEXT             7

/*
 * mpgedit_edit_files state machine states
 */
enum mpgedit_edit_files_states {
  MPGEDIT_STATE_INIT,
  MPGEDIT_STATE_EDIT_INIT,
  MPGEDIT_STATE_EDITING,
  MPGEDIT_STATE_EDIT_DONE
};

typedef enum mpgedit_edit_index_errors {
    MPGEDIT_EDIT_INDEX_STS_OK = 0,
    MPGEDIT_EDIT_INDEX_STS_FAIL_CALLOC,
    MPGEDIT_EDIT_INDEX_STS_DIR_READONLY,
    MPGEDIT_EDIT_INDEX_STS_INDEX_EXISTS
} mpgedit_edit_index_errors_en;

typedef struct _mpgedit_t mpgedit_t;

/*
 * Deliberate incomplete type specification, so the outside world
 * has no idea how to access this structure.  It is just a context handle.
 */
typedef struct _editspec_t editspec_t;

_DSOEXPORT int  _CDECL mpgedit_edit_files(
                                   mpgedit_t *editctx,
                                   int        *rstatus);

_DSOEXPORT mpgedit_t * _CDECL mpgedit_edit_files_init(
                                   editspec_t *edarray,
                                   char     *outfile,
                                   unsigned int flags,
                                   int      *rstatus);

_DSOEXPORT mpgedit_t * _CDECL mpgedit_edit_files_init5(
                                   editspec_t      *edarray,
                                   char            *outfile,
                                   unsigned int    flags,
                                   mpeg_file_stats *edit_stats,
                                   int             *rstatus);

_DSOEXPORT void  _CDECL mpgedit_edit_files_free(void *editctx);
_DSOEXPORT void  _CDECL mpgedit_free(void *ptr);

/* Refactor mpgedit_edit_index_init().  The issue here is we need access
 * to the context after EOF, so we can print the file statistics.  For this
 * reason, we need a context constructor, and destructor.  This is a 
 * cleaner implementation as well
 */

 /*
  * Same functionality as the old API, plus return the status code, rsts.
  * rsts == 1: failed to calloc playctx
  *      == 2: failed to allocate index file name; read-only directory
  *      == 3: Index file already exists
  */
#define _MPGEDIT_INDEX_NEW_ERR_ALLOC      1
#define _MPGEDIT_INDEX_NEW_ERR_DIR_RDONLY 2
#define _MPGEDIT_INDEX_NEW_INDEX_EXISTS   3

_DSOEXPORT void * _CDECL mpgedit_edit_index_new(char *name, int *rsts);
_DSOEXPORT void * _CDECL mpgedit_edit_index_init(char *name);
_DSOEXPORT int    _CDECL mpgedit_edit_index(void *editctx);
_DSOEXPORT void   _CDECL mpgedit_edit_index_free(void *editctx);

_DSOEXPORT long   _CDECL mpgedit_edit_index_sec(void *editctx);
_DSOEXPORT long   _CDECL mpgedit_edit_index_msec(void *editctx);
_DSOEXPORT long   _CDECL mpgedit_edit_index_frames(void *editctx);
_DSOEXPORT long   _CDECL mpgedit_edit_index_offset(void *editctx);
_DSOEXPORT int    _CDECL mpgedit_edit_index_get_stats(void *ifctx);
 
_DSOEXPORT long   _CDECL mpgedit_edit_frames(void *editctx);
_DSOEXPORT long   _CDECL mpgedit_edit_sec(void *editctx);
_DSOEXPORT long   _CDECL mpgedit_edit_offset(void *editctx);
_DSOEXPORT int    _CDECL mpgedit_edit_has_xing(void *editctx);
_DSOEXPORT void * _CDECL mpgedit_edit_xing_header(void *editctx);


_DSOEXPORT editspec_t * _CDECL mpgedit_editspec_init(void);
_DSOEXPORT void         _CDECL mpgedit_editspec_free(editspec_t *);
_DSOEXPORT int          _CDECL mpgedit_editspec_append(editspec_t *ctx,
                                                       const char *file,
                                                       const char *timespec);
_DSOEXPORT editspec_t * _CDECL mpgedit_editspec_get_edit(editspec_t *ctx,
                                                         int start, int len,
                                                         int append, 
                                                         int *ret_len);
_DSOEXPORT char       * _CDECL mpgedit_editspec_get_file(editspec_t *ctx,
                                                         int indx);
_DSOEXPORT long         _CDECL mpgedit_editspec_get_length(editspec_t *ctx);
_DSOEXPORT mpeg_time  * _CDECL mpgedit_editspec_get_stime(editspec_t *ctx, 
                                                          int indx);
_DSOEXPORT mpeg_time  * _CDECL mpgedit_editspec_get_etime(editspec_t *ctx, 
                                                          int indx);
_DSOEXPORT long         _CDECL mpgedit_editspec_get_indexed(editspec_t *ctx, 
                                                            int indx);
_DSOEXPORT void         _CDECL mpgedit_editspec_set_indexed(editspec_t *ctx, 
                                                            int indx,
                                                            int value);

_DSOEXPORT char *       _CDECL mpgedit_edit_make_output_filename(char *dir,
                                        char *base,
                                        char *ext,
                                        int *index);

_DSOEXPORT mpeg_file_stats * _CDECL mpgedit_edit_stats_ctx(void *editctx);

_DSOEXPORT mpeg_time  * _CDECL mpgedit_edit_total_length_time(void *editctx);

_DSOEXPORT mpeg_header_data * _CDECL mpgedit_edit_frame_header(void *editctx);

_DSOEXPORT int          _CDECL mpgedit_edit_times_init(editspec_t *edits);

_DSOEXPORT char *       _CDECL mpgedit_edit_validate_times(editspec_t *edits,
                                                           int start, int cnt);

_DSOEXPORT void _CDECL mpgedit_edit_set_seek_callback(void *_ctx,
                                     int (*status)(void *, long, long),
                                     void *data);

_DSOEXPORT void _CDECL mpgedit_edit_stats2str(mpgedit_t *play_ctx,
                                              mpeg_time *time, 
                                              char *ret_str);

#endif

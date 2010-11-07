/*
 * mpgedit file editing API
 *
 * Copyright (C) 2001-2003 Adam Bernstein
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


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include "portability.h"
#include "mp3_header.h"
#include "mp3time.h"
#include "editif.h"
#include "xing_header.h"
#include "xingedit.h"
#include "mpegfio.h"
#include "mpegindx.h"
#include "mpegstat.h"
#include "md5lib/md5.h"

#include "p_playif.h"
#define _EDITIF_C
#include "p_playif.h"


typedef struct _editspec_body
{
    char      *filename;
    char      *timespec;
    mpeg_time stime;
    mpeg_time etime;
    int       indexed;
} editspec_body;


/* Finally define structure for incomplete type defined in editif.h */
struct _editspec_t
{
    editspec_body *body;
    int           alloc_len;
    int           indx;
    int           length;
};


/* 
 * This needs to be make public, or just use mpgedit_edit_files().
 */
static int mpegfio_edit_segment(void            *in_edctx,
                                FILE            *infp,
                                FILE            *outfp,
                                mpeg_time       *stime,
                                mpeg_time       *etime,
                                int             single_frame,
                                mpeg_header_data *stats_header,
                                mpeg_file_stats *stats)
{
    int              found = 0;
    int              tnow  = 0;
    long             sec;
    long             usec;
    long             etime_sec;
    long             etime_usec;
    int              eof = 0;
    playctx          *ctx;
    playctx          ctx_data;
    int              next_sec = 0;
    md5_state_t      md5state;
    int              sts;


    if (!in_edctx) {
        memset(&ctx_data, 0, sizeof(ctx_data));
        ctx = &ctx_data;
        ctx->stime = *stime;
        ctx->etime = *etime;
    }
    else {
        ctx = (playctx *) in_edctx;
        if (!ctx->inited) {
            ctx->inited = 1;
            ctx->stime  = *stime;
            ctx->etime  = *etime;
        }
    }
    memset(stats_header, 0, sizeof(*stats_header));

    mpeg_time_gettime(&ctx->etime, &etime_sec, &etime_usec);
    do {
        found = read_next_mpeg_frame(infp, &ctx->mpegiobuf, stats_header, &eof);
        if (found == 1) {
            mpeg_time_frame_increment(&ctx->stime, stats_header);
            mpeg_file_stats_gather(stats, stats_header);
            if (ctx->do_md5sum) {
                stats_header->do_md5sum = ctx->do_md5sum;
                md5_init(&md5state);
                md5_append(
                    &md5state, 
                    (md5_byte_t *) mpeg_file_iobuf_getptr(&ctx->mpegiobuf),
                    stats_header->frame_size);
                md5_finish(&md5state, stats_header->md5sum);
            }
            tnow = mpeg_time_2seconds(&ctx->stime);
            mpeg_time_gettime(&ctx->stime, &sec, &usec);

            if (outfp) {
                sts = fwrite(mpeg_file_iobuf_getptr(&ctx->mpegiobuf),
                             stats_header->frame_size, 1, outfp);
                if (sts != 1) {
                    return -1;
                } 

            }

#if 0
printf("sec=%d:%d usec=%d:%d\n", sec, etime_sec, usec, etime_usec);
#endif
            if (sec > etime_sec || (sec == etime_sec && usec >= etime_usec)) {
                eof = 1;
            }
            if (tnow > ctx->tlast || eof) {
                ctx->tlast = tnow;
                next_sec = 1;
            }
            mpeg_file_iobuf_setstart(&ctx->mpegiobuf,
                                     mpeg_file_iobuf_getstart(&ctx->mpegiobuf) +
                                     stats_header->frame_size);
        }
        /* found */
    } while (!single_frame && !eof && (!in_edctx || (in_edctx && !next_sec)));
    *stime = ctx->stime;
    if (eof) {
        ctx->inited = 0;
        ctx->tlast  = 0;
        mpeg_file_iobuf_clear(&ctx->mpegiobuf);
    }
    return eof;
}


/*
 * Helper function for mpgedit_edit_index_init().  Calls 
 * mp3edit_create_indexfile() continue file indexing when ctx->index_ctx
 * is not NULL. Otherwise, the extence of the the index file is
 * tested, and when not found, an index creation context is allocated.
 */ 
static int create_timeindex_file(playctx *ctx)
{
    FILE *indxfp    = NULL;
    int  eof        = 0;
    int  sts        = 0;

    if (ctx->indxfp) {
        /*
         * Iterative indexing in progress. This operation continues
         * until mp3edit_create_indexfile() returns eof.
         */
        eof = mp3edit_create_indexfile(ctx);
        if (eof) {
            /*
             * Close write-only file so it can be opened again read-only
             */
            fclose(ctx->indxfp);
            rewind(ctx->mpegfp);
            ctx->indxfp = fopen(ctx->indx_name, "rb");
        }
        goto clean_exit;
    }

    /*
     * When the index file exists, open for read-only, and indexing
     * complete (eof status).  Otherwise, initialize iterative
     * index context.
     */
    if (access(ctx->indx_name, F_OK) == -1) {
        indxfp = fopen(ctx->indx_name, "wb");
        if (!indxfp) {
            /*
             * This is bad.  Return eof status so caller
             * will not be stuck in an infinite loop.
             */
            eof = 1;
            goto clean_exit;
        }

        ctx->indxfp   = indxfp;
    }
    else {
        ctx->indxfp = fopen(ctx->indx_name, "rb");
        eof = 1;
    }

clean_exit:
    if (sts) {
        if (indxfp) {
            fclose(indxfp);
        }
    }
    return eof;
}


static editspec_t *___mpgedit_editspec_alloc(size_t len)
{
    editspec_t *ctx;

    ctx = calloc(1, sizeof(editspec_t));
    if (ctx) {
        ctx->body = calloc(len, sizeof(editspec_body));
        if (ctx) {
            ctx->alloc_len = len;
        }
    }
    return ctx;
}


editspec_t *mpgedit_editspec_init(void)
{
    return ___mpgedit_editspec_alloc(32);
}


void mpgedit_editspec_free(editspec_t *ctx)
{

    if (!ctx) {
        return;
    }
    if (ctx->indx > 0) {
        free(ctx->body);
    }
    free(ctx);
}

char *_normalize_path_name(const char *file)
{
    char *path_dir = NULL;
    char *path_base = "";
    char *path_ext  = "";
    char *filename;
    char *filenamep = NULL;

    /* If file ends with one of these extensions, change to .mp3 */
    path_dir = mpgedit_pathname_parse(file, &path_base, &path_ext);
    if (path_ext && path_dir && path_base &&
        (!strcmp(path_ext, "lvl") || !strcmp(path_ext, "idx")))
    {
        filename = filenamep =
            (char *) malloc(strlen(path_dir) + strlen(path_base) + 6);
        if (filename) {
            strcpy(filename, path_dir);
            if (*path_base) {
                strcat(filename, "/");
                strcat(filename, path_base);
                strcat(filename, ".mp3");
            }
        }
        if (path_dir) {
            free(path_dir);
        }
        if (path_base) {
            free(path_base);
        }
        if (path_ext) {
            free(path_ext);
        }
    }
    else {
        filename = (char *) file;
    }
    if (filenamep) {
        free(filenamep);
    }
    return filename;
}

int  mpgedit_editspec_append(editspec_t *ctx,
                             const char *file, 
                             const char *timespec)
{
    char *cp;
    editspec_body *newbody;
    long sec;
    long usec;
    char *filename;
    int i;

    if (!ctx) {
        return -1;
    }
    if ((ctx->indx+1) >= ctx->alloc_len) {
        newbody = realloc(ctx->body,
                          ctx->alloc_len * 2 * sizeof(editspec_body));
        if (!newbody) {
            return -1;
        }
        memset(&newbody[ctx->alloc_len], 0, 
               ctx->alloc_len * sizeof(editspec_body));
        ctx->body = newbody;
        ctx->alloc_len *= 2;
    }
    
    /*
     * Incrementing indx is maybe a little too tricky.  When there already is
     * a edit time specification presesrve by incrementing indx.  When there
     * is a file name specification set the name, and move on to the next
     * entry.
     */
    if (timespec && timespec[0]) {
        if (ctx->body[ctx->indx].timespec) {
            if (file) {
                filename = _normalize_path_name(file);
                ctx->body[ctx->indx].filename = strdup(filename);
                file = NULL;
            }
            ctx->indx++;
        }
        ctx->body[ctx->indx].timespec = cp = strdup(timespec);
        if (cp[0] != '-') {
            mpeg_time_string2time(cp, &ctx->body[ctx->indx].stime);
        }
        
        cp = strrchr(cp, '-');
        if (cp) {
            mpeg_time_string2time(cp+1, &ctx->body[ctx->indx].etime);
            mpeg_time_gettime(&ctx->body[ctx->indx].etime, &sec, &usec);
            if (sec == 0 && usec == 0) {
                mpeg_time_init(&ctx->body[ctx->indx].etime,
                               MP3_TIME_INFINITE, 0);
            }
        }
        else {
            mpeg_time_init(&ctx->body[ctx->indx].etime, MP3_TIME_INFINITE, 0);
        }
    }
    if (file) {
        /* If file ends with one of these extensions, change to .mp3 */
        filename = _normalize_path_name(file);
        ctx->body[ctx->indx].filename = strdup(filename);

        ctx->indx++;
        ctx->length++;
    }

    /*
     * Search for filename "holes", and fill them in with the last specified
     * filename. mpgedit -e 2-4 -e 3-6 -f test1.mp3 leaves such a hole.
     */
    cp = NULL;
    for (i=ctx->indx-1; i>=0; i--) {
        if (ctx->body[i].filename) {
            cp = ctx->body[i].filename;
        }
        if (cp && !ctx->body[i].filename) {
            ctx->body[i].filename = strdup(cp);
            ctx->length++;
        }
    }

    return 0;
}


void mpgedit_editspec_set_indexed(editspec_t *ctx, int  indx, int  value)
{
    if (ctx && indx < ctx->indx) {
        ctx->body[indx].indexed = value;
    }
}


char *mpgedit_editspec_get_file(editspec_t *ctx, int indx)
{
    if (!ctx || indx >= ctx->indx) {
        return NULL;
    }
    return ctx->body[indx].filename;
}


mpeg_time *mpgedit_editspec_get_stime(editspec_t *ctx, int indx)
{
    if (!ctx || indx >= ctx->indx) {
        return NULL;
    }
    return &ctx->body[indx].stime;
}


mpeg_time *mpgedit_editspec_get_etime(editspec_t *ctx, int indx)
{
    if (!ctx || indx >= ctx->indx) {
        return NULL;
    }
    return &ctx->body[indx].etime;
}


long mpgedit_editspec_get_indexed(editspec_t *ctx, int indx)
{
    if (!ctx || indx >= ctx->indx) {
        return -1;
    }
    return ctx->body[indx].indexed;
}


long mpgedit_editspec_get_length(editspec_t *ctx)
{
    if (!ctx) {
        return -1;
    }
    return ctx->length;
}


/*
 * Form an editspec array of contiguous edits from an input
 * editspec.  Example:
 * Input editspec: 0-5/f1.mp3 10-25/f1.mp3 25-45/f2.mp3 55-/f2.mp3
 * Output editspec: 0-5/f1.mp3 10-25/f1.mp3
 * Second call:     25-45/f2.mp3 55-/f2.mp3
 */
editspec_t *mpgedit_editspec_get_edit(editspec_t *edarray, 
                                      int        start,
                                      int        len,
                                      int        append,
                                      int        *ret_len)
{
    int i;
    int j;
    int rlen=0;
    int stop=0;
    editspec_t *rarray = NULL;

    if (!edarray) {
        return NULL;
    }
    if (append) {
        rlen = len-start;
    }
    else {
        /*
         * Count number of edit entries that end in a zero value. 
         */
        for (i=start, stop=0; i<len && stop==0; i++) {
            rlen = i+1;
            if ((edarray->body[i].etime.units &&
                 edarray->body[i].etime.units != MP3_TIME_INFINITE) ||
                edarray->body[i].etime.usec)
            {
                stop = 1;
            }
            else if ((i+1)<len &&
                     (edarray->body[i+1].stime.units ||
                      edarray->body[i+1].stime.usec))
            {
                stop = 1;
            }
            else if (i==len) {
                stop = 1;
            }
        }
    }

#ifdef _MPGEDIT_DEBUG
    printf("mpgedit_edit_build_editspec: edit entries = %d\n", rlen);
#endif
    rarray = ___mpgedit_editspec_alloc(rlen+1);
    if (rarray) {
        for (i=start, j=0; i<rlen; i++, j++) {
            rarray->body[j] = edarray->body[i];
            if (rarray->body[j].etime.units == 0 &&
                rarray->body[j].etime.usec == 0) 
            {
                rarray->body[j].etime.units = MP3_TIME_INFINITE;
                
            }
        }
        rarray->indx = rlen - start;
        rarray->length = rlen - start;
    }
    if (ret_len) {
        *ret_len = rlen - start;
    }
    return rarray;
}


/*
 * Build a file named base_nnn.ext, where nnn is the index file value
 */
char *mpgedit_edit_make_output_filename(char *dir,
                                        char *base,
                                        char *ext,
                                        int *index)
{
    char *file;
    char *cp;
    char *cp2 = NULL;
    int  dirlen;
    int  baselen;
    int  extlen;
    char fmtbuf[12];

    /*
     * Use file extension provided in base name, otherwise use ext
     */
    cp = strrchr(base, '.');
    if (cp && cp[1]) {
        ext = cp + 1;
        cp2 = strdup(base);
        cp2[cp - base] = '\0';
        base = cp2;
    }

    dirlen  = dir  ? strlen(dir)  : 0;
    baselen = base ? strlen(base) : 0;
    extlen  = ext  ? strlen(ext)  : 0;

    /*
     * Note: sizeof is larger than needed, so the trailing NUL is covered.
     */
    file = malloc(dirlen + sizeof("/") + baselen + 
                  sizeof("_") + 11 + sizeof(".") + extlen);
    if (file) {
        file[0] = '\0';
        if (dir) {
            strcat(file, dir);
            strcat(file, "/");
        }
        if (base) {
            strcat(file, base);
        }
        if (index) {
            strcat(file, "_");
            sprintf(fmtbuf, "%d", *index);
            strcat(file, fmtbuf);
            (*index)++;
        }
        if (ext) {
            strcat(file, ".");
            strcat(file, ext);
        
        }
    }
    if (cp2) {
        free(cp2);
    }
    return file;
}



/*
 * Returns the number of frames processed during an indexing operation.
 */
long mpgedit_edit_index_frames(void *ctx)
{
    if (!ctx) {
        return -1;
    }
    return ((playctx *) ctx)->frame_cnt;
}


/*
 * Returns the file play time, in seconds, processed during
 * an indexing operation.
 */
long mpgedit_edit_index_sec(void *ctx)
{
    if (!ctx) {
        return -1;
    }
    return ((playctx *) ctx)->cursec;
}


/*
 * Returns the file play time millisecond residual for
 * an indexing operation.
 */
long mpgedit_edit_index_msec(void *ctx)
{
    if (!ctx) {
        return -1;
    }
    return ((playctx *) ctx)->curusec/1000;
}


/*
 * Returns the file position, in bytes, during
 * an indexing operation.
 */
long mpgedit_edit_index_offset(void *ctx)
{
    if (!ctx) {
        return -1;
    }
    return ((playctx *) ctx)->file_position;
}


long mpgedit_edit_frames(void *ctx)
{
    if (!ctx) {
        return -1;
    }
    return ((playctx *) ctx)->stats.num_frames;
}


long mpgedit_edit_sec(void *ctx)
{
    /*
     * adam/TBD:
     * Some serious refactoring work still needs to be done.
     * Take a look at the structure members:
     *
     *  ((playctx *) ctx)->stime.units;
     *  ((editfile_ctx *)((playctx *) ctx)->edit_ctx)->stime.units;
     *
     * How are they fundamentally different?
     * Can these be collapsed into the same structure?
     */
    if (!ctx) {
        return -1;
    }
    return ((playctx *) ctx)->stime.units;
}


long mpgedit_edit_offset(void *ctx)
{
    return ctx ? ((playctx *) ctx)->stats.file_size : -1;
}


mpeg_file_stats *mpgedit_edit_stats_ctx(void *ctx)
{
    return ctx ? &((playctx *) ctx)->stats : NULL;
}


mpeg_time *mpgedit_edit_total_length_time(void *ctx)
{
    return ctx ? &((playctx *) ctx)->mpegtval : NULL;
}

mpeg_header_data *mpgedit_edit_frame_header(void *ctx)
{
    return ctx ? &((playctx *) ctx)->stats_frame_header : NULL;
}


void *mpgedit_edit_xing_header(void *ctx)
{
    return ctx ? &((playctx *) ctx)->xingh : NULL;
}


int mpgedit_edit_has_xing(void *ctx)
{
    return ctx ? ((playctx *) ctx)->has_xing : -1;
}


/*
 * Initializes the MP3 index create context, when no index file exists.
 * Returns NULL when the index already exists, or when the indexing
 * operation completes in one iteration. 
 *
 * Continue to call mpgedit_edit_index(ctx) when a non-null value is returned
 * from this function.  mpgedit_edit_index() returns TRUE when indexing is
 * complete. This iterative behavior is required for integration into GUI
 * applications.
 */

 /*
  * Same functionality as the old API, plus return the status code, rsts.
  * rsts == 1: failed to calloc playctx
  *      == 2: failed to allocate index file name; read-only directory
  *      == 3: Index file already exists
  */


void *mpgedit_edit_index_new(char *name, int *rsts)
{
    playctx     *ctx = NULL;
    FILE        *fp = NULL;
    struct stat sbuf;
    int         sts = MPGEDIT_EDIT_INDEX_STS_OK;


    if (!name || !name[0]) {
        return NULL;
    }

    if (stat(name, &sbuf) == -1) {
        return NULL;
    }

    if (!S_ISREG(sbuf.st_mode)) {
        return NULL;
    }

    fp = fopen(name, "rb");
    if (!fp) {
#ifdef _MPGEDIT_DEBUG
        fprintf(stderr, "fopen: test1.mp3 failed %s\n", strerror(errno));
#endif
        return NULL;
    }

    ctx = calloc(1, sizeof(playctx));
    if (!ctx) {
        sts = MPGEDIT_EDIT_INDEX_STS_FAIL_CALLOC;
        goto clean_exit;
    }
    mpeg_file_stats_init(&ctx->stats, name, 0, 0);
    ctx->name   = name;
    ctx->mpegfp = fp;

    ctx->indxfp = mpgedit_indexfile_init(ctx->mpegfp, ctx->name, 
                                         &ctx->indx_name);
    if (!ctx->indx_name) {
        /*
         * The return index file name will not be set when the local directory
         * is read-only.
         */
        sts = MPGEDIT_EDIT_INDEX_STS_DIR_READONLY;
        goto clean_exit;
    }
    if (ctx->indxfp) {
        sts = MPGEDIT_EDIT_INDEX_STS_INDEX_EXISTS;
        goto clean_exit;
    }
    else {
        /*
         * File indexing has not finished. Return the index context to the
         * caller, so indexing can continue iteratively.
         */
        if (!create_timeindex_file(ctx)) {
            return ctx;
        }
    }

clean_exit:
    if (sts && sts != MPGEDIT_EDIT_INDEX_STS_INDEX_EXISTS) {
        if (ctx) {
            if (ctx->indxfp) {
                fclose(ctx->indxfp);
            }
            if (ctx->mpegfp) {
                fclose(ctx->mpegfp);
            }
            if (ctx->indx_name) {
                free(ctx->indx_name);
            
            }
            free(ctx);
            ctx = NULL;
        }
        else if (fp) {
            fclose(fp);
        }
    }
    if (rsts) {
        *rsts = sts;
    }

/* adam/TBD: Shouldn't ctx always be NULL here?? */
    return ctx;
}


/*
 * Preserve the old API
 */
void *mpgedit_edit_index_init(char *name)
{
    return mpgedit_edit_index_new(name, NULL);
}


/*
 * Iterative index file creation function.  Call this function after
 * calling mpgedit_edit_index_init first(). Continue to call this
 * function until TRUE is returned.
 */
int mpgedit_edit_index(void *play_ctx)
{
    playctx     *ctx = (playctx *) play_ctx;
    int         eof = 1;

    if (ctx) {
        /*
         * Iterative index file creation in progress. 
         */
        eof = create_timeindex_file(ctx);
    }
    return eof;
}


/*
 * Free context created by mpgedit_edit_index_init();
 */
void mpgedit_edit_index_free(void *play_ctx)
{
    playctx     *ctx = (playctx *) play_ctx;

    if (ctx) {
        if (ctx->mpegfp) {
            fclose(ctx->mpegfp);
        }
        if (ctx->indxfp) {
            fclose(ctx->indxfp);
        }
        if (ctx->indx_name) {
            free(ctx->indx_name);
        }
        free(ctx);
        ctx = NULL;
    }
}


/* Formerly known as init_edit_times */
int mpgedit_edit_times_init(editspec_t *edits)
{
    char *name = NULL;
    char *sp;
    char *ep;
    long sec;
    long usec;
    int  i;
    int status = 1;
    char *tmptimespec;
    int cnt;

    cnt = mpgedit_editspec_get_length(edits);
    for (i=cnt-1; i>=0; i--) {
        sec = usec = 0;

        if (edits->body[i].timespec) {
            /*
             * Multiple time specs can be specified before the file name.
             * Associate all such specifications with the filename
             * that follows.
             */
            if (edits->body[i].filename) {
                name = edits->body[i].filename;
            }
            else {
                if (name) {
                    edits->body[i].filename = strdup(name);
                }
            }

            /* Convert edit begin/end times into time structures */
            tmptimespec = strdup(edits->body[i].timespec);
            sp = tmptimespec;
            ep = strchr(sp, '-');
            if (ep) {
                *ep++ = '\0';
            }
            if (sp && *sp) {
                mpeg_time_string2time(sp, &edits->body[i].stime);
            }
            if (ep && *ep) {
                mpeg_time_string2time(ep, &edits->body[i].etime);
            }
            else {
                mpeg_time_init(&edits->body[i].etime, MP3_TIME_INFINITE, 0);
            }
            free(tmptimespec);
        }
        else {
            mpeg_time_init(&edits->body[i].etime, MP3_TIME_INFINITE, 0);
            edits->body[i].timespec = strdup("");
        }
    }
    if (!edits->body[0].filename) {
        status = 0;
    }
    return status;
}


static char *validate_edit_times_errorstr(char *filename,
                                          char *timespec, int status)
{
    char *errstr = malloc(128+strlen(filename));

    *errstr = '\0';
    switch(status) {
      case 1:
        sprintf(errstr, "file name '%s' invalid", filename);
        break;
      case 2:
      case 3:
        sprintf(errstr, "file '%s', bad start time '%s'", filename, timespec);
        break;
      case 4:
        sprintf(errstr, "file '%s', start time is larger than end time '%s'",
                filename, timespec);
        break;
      case 5:
        sprintf(errstr, "file '%s', bad end time '%s'", filename, timespec);
        break;
      default:
        break;
    }
    return errstr;
}


/*
 * Seek to the start and end time for each edit contained in the editspec
 * array.  Calling this function will create the index file for the
 * corresponding mp3 file, when one does not exist.
 */
char *mpgedit_edit_validate_times(editspec_t *edits, int start, int cnt)
{
    FILE                *fp = NULL;
    FILE                *indxfp = NULL;
    int                 i;
    int                 rsts = 1;
    mpeg_time           mpegtval;
    int                 status;
    long                spos = 0;
    long                epos = 0;
    char                *errstr = NULL;
    int                 edits_cnt;
    int                 end;

    edits_cnt = mpgedit_editspec_get_length(edits);
    memset(&mpegtval, sizeof(mpegtval), 0);
    end = start+cnt;
    if (cnt > edits_cnt || cnt == -1) {
        end = edits_cnt;
    }
    for (i=start; i<end; i++) {
        /*
         * Illegal to have start greater than end time
         */
        if ((edits->body[i].stime.units > edits->body[i].etime.units) ||
            (edits->body[i].stime.units == edits->body[i].etime.units && 
             edits->body[i].stime.usec > edits->body[i].etime.usec)) 
        {
            rsts = 4;
            goto clean_exit;
        }

        fp = fopen(edits->body[i].filename, "rb");
        if (!fp) {
            goto clean_exit;
        }

        indxfp = mpgedit_indexfile_init(fp, edits->body[i].filename, NULL);
        if (fseek(fp, 0, SEEK_END) != -1) {
            epos = ftell(fp);
        }
        status = mpeg_time_seek_starttime(fp, indxfp,
                                          &edits->body[i].stime, &mpegtval, 0);
        if (status == -1) {
            rsts = 3;
            goto clean_exit;
        }
        spos = ftell(fp);

        if (spos == epos) {
            rsts = 5;
            goto clean_exit;
        }
        else if (edits->body[i].etime.units != MP3_TIME_INFINITE) {
            status = mpeg_time_seek_starttime(fp, indxfp,
                                              &edits->body[i].etime, 
                                              &mpegtval, 0);
        }
        if (status == -1) {
            rsts = 5;
            goto clean_exit;
        }
        if (fp) {
            fclose(fp);
            fp = NULL;
        }
        if (indxfp) {
            fclose(indxfp);
            indxfp = NULL;
        }
    }
    rsts = 0;

clean_exit:
    if (fp) {
        fclose(fp);
    }
    if (indxfp) {
        fclose(indxfp);
    }
    if (rsts) {
        errstr = validate_edit_times_errorstr(edits->body[i].filename,
                                              edits->body[i].timespec,
                                              rsts);
    }
    return errstr;
}


mpgedit_t *mpgedit_edit_files_init5(editspec_t      *edarray,
                                    char            *outfile,
                                    unsigned        int flags,
                                    mpeg_file_stats *edit_stats,
                                    int             *rstatus)
{
    playctx          *ctx;
    int              append;
    int              status;

    /*
     * Context initialization.
     */
    ctx = (void *) calloc(1, sizeof(playctx));

    if (!ctx) {
        *rstatus = MPGEDIT_EDIT_FILES_BAD_CONTEXT;
        return NULL;
    }
    ctx->g_stats = edit_stats;
    append = flags & MPGEDIT_FLAGS_APPEND;
    ctx->do_md5sum = flags & MPGEDIT_FLAGS_MD5SUM_FRAME;
    if (ctx->state == MPGEDIT_STATE_INIT) {
        ctx->state = MPGEDIT_STATE_EDIT_INIT;
        if (append) {
            /*
             * Test if file already exists.  If not, turn off append flag.
             */
            status = validate_outfile(outfile, 0, NULL);
            if (status == 0) {
                flags ^= MPGEDIT_FLAGS_APPEND;
                append = 0;
            }
        }
        status = validate_outfile(outfile, append, NULL);
        if ((!append && status) || (append && status == 2)) {
            switch (status) {
              case 1: 
                *rstatus = MPGEDIT_EDIT_FILES_ERR_OUTFILE_EXISTS;
                break;
              case 2: 
              default:
                *rstatus = MPGEDIT_EDIT_FILES_ERR_OUTFILE_CANT_APPEND;
                break;
            }
            goto clean_exit;
        }
        ctx->editsindx = 0;
        /* 
         * Save call arguments into context for next iterative call use.
         * We have to free edarray, and outfile memory when done iterating.
         */
        ctx->edarray = edarray;
        ctx->edlen   = edarray->indx;
        ctx->outfile = outfile;
        ctx->flags   = flags;
    }
    *rstatus = 0;

clean_exit:
    if (*rstatus) {
        free(ctx);
        ctx = NULL;
    }
    return (mpgedit_t *) ctx;
}


mpgedit_t *mpgedit_edit_files_init(editspec_t *edarray,
                                   char     *outfile,
                                   unsigned int flags,
                                   int      *rstatus)

{
    mpeg_file_stats edit_stats;

    memset(&edit_stats, 0, sizeof(edit_stats));
    return mpgedit_edit_files_init5(edarray, outfile, flags, &edit_stats, rstatus);
}


void mpgedit_edit_files_free(void *play_ctx)
{
    if (play_ctx) {
        free(play_ctx);
    }
}


void mpgedit_free(void *ptr)
{
    if (ptr) {
        free(ptr);
    }
}


void mpgedit_edit_set_seek_callback(void *_ctx,
                                    int (*status)(void *, long, long),
                                    void *data)
{
    playctx *ctx = ((playctx *) _ctx);

    if (!ctx) {
        return;
    }
    ctx->fp_index = status;
    ctx->dp_index = data;
}


long mpeg_time_read_seek_starttime(playctx *ctx, mpeg_time *etime)
{
    long sec,  usec;
    long tsec = 0, tusec = 0;
    int  eof  = 0;
    int  stop = 0;
    int  found = -1;
    long tnext = 1;
    long seek_file_pos = 0;

    if (ctx->has_xing) {
        fseek(ctx->mpegfp, ctx->has_xing, SEEK_SET);
    }
    else {
        fseek(ctx->mpegfp, 0, SEEK_SET);
    }

    mpeg_time_gettime(&ctx->stime, &sec, &usec);
    if (tsec > sec || (tsec == sec && tusec >= usec)) {
        stop = 1;
    }
    while (!stop && !eof) {
        found = read_next_mpeg_frame(ctx->mpegfp, &ctx->mpegiobuf,
                                     &ctx->header, &eof);
        if (found == 1) {
            ctx->frame_cnt++;
            mpeg_time_frame_increment(&ctx->mpegtval, &ctx->header);
            mpeg_time_gettime(&ctx->mpegtval, &tsec, &tusec);
            if (tsec > sec || (tsec == sec && tusec >= usec)) {
                stop = 1;
            }
            mpeg_file_iobuf_setstart(
                &ctx->mpegiobuf,
                mpeg_file_iobuf_getstart(&ctx->mpegiobuf) +
                                         ctx->header.frame_size);
            if (tsec >= tnext) {
                tnext = tsec + 1;
                if (ctx->fp_index) {
                    ctx->fp_index(ctx->dp_index, tsec, tusec);
                }
            }
        }
    }
    mpeg_time_init(etime, tsec, tusec);

    /*
     * Seek back to the portion of the stream which has not
     * been skipped over.  Remember mpegiobuf buffers data from the
     * input file in 4K blocks, so it is certain there will be a
     * residual that must be kept.
     */
     if (found != -1) {
         fseek(ctx->mpegfp,
               ctx->mpegiobuf.start - ctx->mpegiobuf.len, SEEK_CUR);
     }
     if (ctx->mpegfp) {
         seek_file_pos = ftell(ctx->mpegfp);
     }

    /*
     * Cheesy, but must signal EOF to callback in some way.
     */
    if (ctx->fp_index && eof) {
        ctx->fp_index(ctx->dp_index, -1, -1);
    }

    return seek_file_pos;
}



/*
 * Edit list of mp3 files "edarray",  writing output to "outfile".
 * Edits are appended to the end of "outfile" when "append" is true.
 */
int mpgedit_edit_files(mpgedit_t *play_ctx, int *rstatus)
{
    mpeg_header_data append_header;
    XHEADDATA        append_xingh;

    int              status = 0;
    playctx          *ctx;
    int              append;
    int              xing_flag;
    long             rsts = 0;
    long             sec;
    long             usec;
    int              single_frame = 0;
    int              input_has_xing = 0;
    char             *outfile = NULL;
    FILE             **editfp = NULL;

    if (!play_ctx || !play_ctx->edarray || play_ctx->edarray->indx == 0) {
        *rstatus = MPGEDIT_EDIT_FILES_BAD_CONTEXT;
        return 0;
    }
    ctx = (playctx *) play_ctx;

    append       = ctx->flags & MPGEDIT_FLAGS_APPEND;
    xing_flag    = ctx->flags &
                       (MPGEDIT_FLAGS_XING_OMIT|MPGEDIT_FLAGS_XING_FIXUP);
    single_frame = ctx->flags & MPGEDIT_FLAGS_EACH_FRAME;

    do {
    switch (ctx->state) {
      case MPGEDIT_STATE_INIT:
        /* Does nothing, silence compiler warning */
        break;

      case MPGEDIT_STATE_EDIT_INIT:
        ctx->mpegfp = fopen(ctx->edarray->body[ctx->editsindx].filename, "rb");
        if (!ctx->mpegfp) {
            *rstatus = MPGEDIT_EDIT_FILES_ERR_INFILE_ERR;
            goto clean_exit;
        }

        ctx->stime = ctx->edarray->body[ctx->editsindx].stime;

        if (!(ctx->flags & MPGEDIT_FLAGS_NO_INDEX)) {
            ctx->indxfp = mpgedit_indexfile_init(
                              ctx->mpegfp,
                              ctx->edarray->body[ctx->editsindx].filename,
                              NULL);
            if (!ctx->indxfp) {
#ifdef _MPGEDIT_DEBUG
                fprintf(stderr, "Error opening time index file\n");
#endif
                *rstatus = MPGEDIT_EDIT_FILES_ERR_INDEXFILE_ERR;
                goto clean_exit;
            }

            rsts = mp3edit_get_size_from_index(ctx->indxfp, &sec, &usec);
            if (rsts != -1) {
                ctx->fsize = rsts;
                ctx->sec   = sec;
                ctx->usec  = usec;
            }
        }


        if ((ctx->flags & MPGEDIT_FLAGS_NO_EDITS) == 0) {
            ctx->outfile_exists = (access(ctx->outfile, F_OK) != -1);

            /*
             * Open edit file when a cut begin/end time are specified.
             * Edit file is opened for read/write, so the Xing VBR header
             * can be modified.
             */
            if (append) {
                if (!ctx->editfp) {
                    ctx->editfp = fopen(ctx->outfile, "ab+");
                }
            }
            else {
                ctx->editfp = fopen(ctx->outfile, "wb");
            }
    
            if (!ctx->editfp) {
#ifdef _MPGEDIT_DEBUG
                fprintf(stdout, "failed to open edit file tmp.mp3\n");
#endif
                *rstatus = MPGEDIT_EDIT_FILES_ERR_OUTFILE_OPEN;
                goto clean_exit;
            }
        }
        else if (ctx->flags & MPGEDIT_FLAGS_GET_STATS) {
            rsts = mp3edit_indexfile_get_stats(ctx);
            if (rsts == 0) {
                ctx->state = MPGEDIT_STATE_EDIT_DONE;
                return 0;
            }
        }

        /*
         * Determine if VBR Xing header is present.  Either pass along this
         * frame to the output file, or skip it, if currently positioned
         * in the middle of the output file.
         */
        memset(&ctx->header, 0, sizeof(ctx->header));
        if (append) {
            ctx->has_xing = mpegfio_has_xing_header(ctx->editfp,
                                                    &ctx->header,
                                                    &ctx->xingh, append);
            if (ctx->has_xing) {
                mpeg_file_stats_init(
                    &ctx->stats,
                    ctx->edarray->body[ctx->editsindx].filename,
                    ctx->outfile_exists ? 0 : ctx->has_xing, 0);
            }
            /* 
             * This is to skip xing header when one is already present on 
             * input file 
             */
            input_has_xing = mpegfio_has_xing_header(ctx->mpegfp,
                                                     &append_header,
                                                     &append_xingh, append);
        }
        else {
            ctx->has_xing = mpegfio_has_xing_header(ctx->mpegfp,
                                                    &ctx->header,
                                                    &ctx->xingh, append);
            /*
             * has_xing contains the size of the xing header. When
             * appending files together, do not count this size in the file
             * total, since the joined file does not have this frame.
             */
            mpeg_file_stats_init(&ctx->stats,
                                 ctx->edarray->body[ctx->editsindx].filename,
                                 ctx->outfile_exists ? 0 : ctx->has_xing, 0);
        }

        if (ctx->has_xing > 0) {
            if (!ctx->g_stats->inited) {
                mpeg_file_stats_init(ctx->g_stats, 0, ctx->has_xing, 0);
                ctx->g_stats->inited = 1;
            }

            /*
             * Never write Xing header prefix when appending output.
             * We are appending when editing after the first iteration,
             * or append is surpressed from the argument list.
             */
            if (ctx->editfp && !append &&
                (xing_flag == MPGEDIT_FLAGS_XING_DEFAULT ||
                 xing_flag == MPGEDIT_FLAGS_XING_FIXUP))
            {
                status = fwrite(ctx->xingh.xingbuf,
                                ctx->has_xing, 1, ctx->editfp);
                if (status != 1) {
                    return -1;
                } 
            }
        }
        else {
            fseek(ctx->mpegfp, 0, SEEK_SET);
        }

        mpeg_time_clear(&ctx->mpegtval);

        if (ctx->flags & MPGEDIT_FLAGS_NO_INDEX) {
            /* Seek to start time without help of index file */
            mpeg_time_read_seek_starttime(ctx, &ctx->mpegtval);
            mpeg_file_iobuf_clear(&ctx->mpegiobuf);
        }
        else {
            /*
             * "has_xing" will skip that many bytes in the input file.
             * Don't use has_xing when appending to an existing output
             * file, and the input file does not have a Xing header. 
             */
            status = mpeg_time_seek_starttime(
                         ctx->mpegfp, ctx->indxfp, &ctx->stime,
                         &ctx->mpegtval, 
                         (append && !input_has_xing) ?  0 : ctx->has_xing);
        }
        if (status == -1) {
#ifdef _MPGEDIT_DEBUG
            fprintf(stderr, "Invalid start time 2\n");
#endif
            *rstatus = MPGEDIT_EDIT_FILES_ERR_START_TIME;
            goto clean_exit;
        }

        ctx->state = MPGEDIT_STATE_EDITING;

        /* fall through to next state */

      case MPGEDIT_STATE_EDITING:

        /* Iterate on this function. Returns TRUE on EOF */
        rsts = mpegfio_edit_segment(ctx,
                                    ctx->mpegfp,
                                    ctx->editfp,
                                    &ctx->mpegtval,
                                    &ctx->edarray->body[ctx->editsindx].etime,
                                    single_frame,
                                    &ctx->stats_frame_header,
                                    &ctx->stats);
        if (rsts==0) {
            *rstatus = 0;
            return 1;
        }

        ctx->state = MPGEDIT_STATE_EDIT_DONE;

        /* fall through to next state */

      case MPGEDIT_STATE_EDIT_DONE:
        mpeg_file_stats_join(ctx->g_stats, &ctx->stats);
        if (ctx->has_xing > 0) {
            if (ctx->editfp && 
                (xing_flag == MPGEDIT_FLAGS_XING_DEFAULT ||
                 xing_flag == MPGEDIT_FLAGS_XING_FIXUP))
            {
                outfile    = ctx->outfile;
                editfp     = &ctx->editfp;
            }
            else if (xing_flag == MPGEDIT_FLAGS_XING_FIXUP) {
/*
 * This is pretty bad.  This violates the rule of the source file is never
 * modified.
 */
                outfile    = ctx->edarray->body[ctx->editsindx].filename;
                editfp     = &ctx->mpegfp;
            }

            if (editfp && outfile) {
                fclose(*editfp);
                *editfp = fopen(outfile, "rb+");
                if (*editfp) {
                    mpgedit_xing_stats_modify_fp(
                        *editfp, &ctx->stats,
                        ctx->outfile_exists ? MPGEDIT_XING_MODIFY_INCR :
                                              MPGEDIT_XING_MODIFY_SET);
                }
            }
        }

        if (ctx->mpegfp) {
            fclose(ctx->mpegfp);
            ctx->mpegfp = NULL;
        }

        if (ctx->indxfp) {
            fclose(ctx->indxfp);
            ctx->indxfp = NULL;
        }

        ctx->editsindx++;
        if (ctx->outfile) {
            append = 1;
        }
        if (!append && ctx->editfp) {
            fclose(ctx->editfp);
            ctx->editfp = NULL;
        }
        ctx->state = MPGEDIT_STATE_EDIT_INIT;
    } /*switch */
    } while (ctx->editsindx < ctx->edlen);
    
    if (ctx->editfp) {
        fclose(ctx->editfp);
        ctx->editfp = NULL;
    }

    /*
     * Set Xing/Info header based on 
     * edited file maximum/minimum bitrate
     */
    if (ctx->has_xing) {
        ctx->g_stats->file_size = 0;
        ctx->g_stats->num_frames = 0;
        mpgedit_xing_stats_modify_file(ctx->outfile, ctx->g_stats, MPGEDIT_XING_MODIFY_INCR);
    }
    
    ctx = NULL;
    *rstatus = 0;

clean_exit:
    return 0;
}


int mpgedit_edit_index_get_stats(void *ifctx)
{
    return mp3edit_indexfile_get_stats(ifctx);
}


/*
 * Same as calling mpeg_file_stats2str() except we dereference
 * play_ctx->stats.  On Win32, obtaining a pointer to play_ctx->stats from
 * mpgedit.dll, then dereferenceing does not work.
 */
void mpgedit_edit_stats2str(mpgedit_t *play_ctx, mpeg_time *time, char *ret_str)
{
    playctx *ctx;

    if (!play_ctx) {
        return;
    }
    ctx = (playctx *) play_ctx;
    mpeg_file_stats2str(&ctx->stats, time, ret_str);
}

/*
 * MPEG audio file time indexing routines
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

#ifndef lint
static char SccsId[] = "$Id: mpegindx.c,v 1.12.2.2 2006/12/27 21:37:56 number6 Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "portability.h"
#include "mpegfio.h"
#include "header.h"
#include "mp3time.h"
#include "mp3_header.h"
#include "xing_header.h"
#include "mpegstat.h"
#include "p_playif.h"
#include "mpegindx.h"

#define MP3EDIT_INDEX_FILE_VERSION  2
#define MP3EDIT_INDEX_FILE_VERSION3 3


#define VALIDATE_OUTFILE_OK           0
#define VALIDATE_OUTFILE_FILE_EXIST   1
#define VALIDATE_OUTFILE_FILE_NOWRITE 2
#define VALIDATE_OUTFILE_NOFILE       3
#define VALIDATE_OUTFILE_DIR_NOEXIST  4
#define VALIDATE_OUTFILE_DIR_NOWRITE  5


int mp3edit_skip_xing_header(FILE *fp)
{
    unsigned char    xing_buf[2048];
    int              cnt;
    int              has_xing = 0;
    mpeg_header_data mpeg_header;
    unsigned char    mpeg_header_data[4];

    memset(xing_buf, 0, sizeof(xing_buf));
    memset(&mpeg_header, 0, sizeof(mpeg_header));

    cnt = fread(mpeg_header_data, sizeof(mpeg_header_data), 1, fp);
    if (cnt != 1) {
        goto clean_exit;
    }

    cnt = decode_mpeg_header(mpeg_header_data, 
                             &mpeg_header, MPGEDIT_ALLOW_MPEG1L1);
    if (!cnt) {
        goto clean_exit;
    }

    memcpy(xing_buf, mpeg_header_data, sizeof(mpeg_header_data));
    cnt = fread(&xing_buf[4], mpeg_header.frame_size - 4, 1, fp);
    if (cnt == 1) {
        has_xing =  xingheader_init(xing_buf, mpeg_header.frame_size, NULL);
    }
    else {
        goto clean_exit;
    }

clean_exit:
    if (!has_xing) {
        rewind(fp);
    }
    return has_xing ? mpeg_header.frame_size : 0;
}


long mp3edit_indexfile_sec(void *ifctx)
{
    playctx *ctx = (playctx *) ifctx;

    return ctx->cursec;
}


long mp3edit_indexfile_msec(void *ifctx)
{
    playctx *ctx = (playctx *) ifctx;

    return ctx->curusec/1000;
}


long mp3edit_indexfile_frames(void *ifctx)
{
    playctx *ctx = (playctx *) ifctx;

    return ctx->frame_cnt;
}

long mp3edit_indexfile_offset(void *ifctx)
{
    playctx *ctx = (playctx *) ifctx;

    return ctx->file_position;
}

/*
 * Create the time index file for an MP3 file. The time index file
 * is required to make fast, and accurate random access into VBR files
 * possible.  Some comments about the format of this file.  There has
 * been some evolution of this file format.  There are two destinct formats
 * supported by the reader of this file, mpeg_time_seek_starttime().
 * Only the modern version of this file is supported for writing.
 *
 * Version "0" format:
 *   | Offset 0 | Offset 1 | Offset 2 | ... | Offset N
 *   where each offset a 4 byte integer in machine byte order.
 *   Each offset is the byte position in the mp3 file for one second of
 *   play time.
 *
 * Version "2" format:
 *  | 0xffffffff | 0x2 | Offset 0 | usec 0 | Offset 1 | usec 1 | ... 
 *  | Offset N | usec N | EOF_Offset N | usec N |
 *   where each entry a 4 byte integer in network byte order.
 *   The first two integers represent the index file version number.  The
 *   first integer has all bits set, the second integer is the format version.
 *   The first integer is all 1s to distinguish from version 0, where the
 *   first time offset is always all zeros.  The remaining values are 
 *   alternating byte offset values, followed by the usec fraction for
 *   the current second.  This fixes the sub-second file positioning bug
 *   present in 0.5 beta, because the microsecond offset for each second
 *   quantum were truncated. All integer values are now in network byte order
 *   (big endian) so index files created on platforms of opposite byte order
 *   can be read.
 *
 * Version "3" format:
 *  | 0xffffffff | 0x3 | Index Header | Offset 0 | usec 0 |
 *    Offset 1 | usec 1 | ...
 * Version 3 format is the same as the version 2 format, except for the
 * addition of the Index Header information.  The size of this block is
 * 100 bytes, the size of 25/4 byte integers. Look at the definition of
 * mpeg_file_stats to understand the size of this header.  All of the integer
 * values in this header are written in network byte order.
 *
 *   Index Header := min_bitrate | max_bitrate | avg_bitrate | file_size |
 *                   num_frames | len_sec | len_msec | bit_rate_array[16]
 *
 * This function is now really a helper function to create_timeindex_file(),
 * which is called from mpgedit_edit_index_init() and mpgedit_edit_index(),
 * both in editif.c.  However, it makes sense to keep this function in this
 * module, as it performs the I/O to the index file, as do other functions
 * in this module.
 */
int mp3edit_create_indexfile(void *ifctx)
{
    playctx          *ctx;
    mpeg_header_data header;
    int32_t          writeint;
    int              found = 0;
    int              eof = 0;
    int              tlast = 0;
    int              indx;

    memset(&header, 0, sizeof(header));

    ctx = (playctx *) ifctx;
    if (ctx->loop || (!ctx->loop && !ctx->inited)) {
        /*
         * Write header version into index file
         */
        writeint = ~0;
        fwrite(&writeint, sizeof(writeint), 1, ctx->indxfp);
        InsertI4(MP3EDIT_INDEX_FILE_VERSION3, (unsigned char *) &writeint);
        fwrite(&writeint, sizeof(writeint), 1, ctx->indxfp);

        /*
         * Write index header placeholder information
         */
        writeint = 0;
        for (indx = 0; indx<25; indx++) {
            fwrite(&writeint, sizeof(writeint), 1, ctx->indxfp);
        }

        /*
         * Skip over Xing header for time counting purposes.
         */
        ctx->file_position = mp3edit_skip_xing_header(ctx->mpegfp);
        if (ctx->file_position) {
            /*
             * The first frame in the input file is the XING header,
             * so include this frame in the index file.
             */
            ctx->inited = 1;
            writeint    = 0;
            fwrite(&writeint, sizeof(writeint), 1, ctx->indxfp);
            fwrite(&writeint, sizeof(writeint), 1, ctx->indxfp);

            /*
             *  Increment statistics file position to include Xing header.
             */
            mpeg_file_stats_init(&ctx->stats, "", ctx->file_position, 0);
        }
    }


    do {
        found = read_next_mpeg_frame(ctx->mpegfp, &ctx->mpegiobuf,
                                     &header, &eof);
        if (found == 1) {
            mpeg_time_frame_increment(&ctx->mpegtval, &header);
            mpeg_file_stats_gather(&ctx->stats, &header);
            ctx->frame_cnt++;
            ctx->tnow = mpeg_time_2seconds(&ctx->mpegtval);
            mpeg_time_gettime(&ctx->mpegtval, &ctx->cursec, &ctx->curusec);
            ctx->file_position += header.position;
            if (!ctx->inited) {
                ctx->inited = 1;
                /*
                 * Second 0 corresponds to the start of the file,
                 * not counting the Xing header, and not counting any junk
                 * in the file before the first valid frame.  Write a 0 usec 
                 * residual as well.
                 */
                InsertI4(ctx->file_position, (unsigned char *) &writeint);
                fwrite(&writeint, sizeof(writeint), 1, ctx->indxfp);
                writeint = 0;
                fwrite(&writeint, sizeof(writeint), 1, ctx->indxfp);
            }
            ctx->file_position += header.frame_size;
            
            if (ctx->tnow > ctx->tlast) {
                ctx->tlast = tlast = ctx->tnow;
                InsertI4(ctx->file_position, (unsigned char *) &writeint);
                fwrite(&writeint, sizeof(writeint), 1, ctx->indxfp);
                InsertI4(ctx->mpegtval.usec, (unsigned char *) &writeint);
                fwrite(&writeint, sizeof(writeint), 1, ctx->indxfp);
            }
            mpeg_file_iobuf_setstart(&ctx->mpegiobuf,
                                    mpeg_file_iobuf_getstart(&ctx->mpegiobuf) +
                                         header.frame_size);
        }
        /* found */
    } while ((ctx->loop || (!ctx->loop && !tlast)) && !eof);

    if (eof) {
        /* 
         * Write EOF Entry. This entry has the file size and the usec time has 
         * the high bit set to signify EOF.
         */
        ctx->file_position = ftell(ctx->mpegfp);
        InsertI4(ctx->file_position, (unsigned char *) &writeint);
        fwrite(&writeint, sizeof(writeint), 1, ctx->indxfp);
        InsertI4(ctx->mpegtval.usec | 1U<<31, (unsigned char *) &writeint);
        fwrite(&writeint, sizeof(writeint), 1, ctx->indxfp);

        /*
         * Write index header information
         */
        fseek(ctx->indxfp, 2 * sizeof(int), SEEK_SET);
    
        /* | min_bitrate | */
        InsertI4(ctx->stats.min_bitrate, (unsigned char *) &writeint);
        fwrite(&writeint, sizeof(writeint), 1, ctx->indxfp);
    
        /* | max_bitrate | */
        InsertI4(ctx->stats.max_bitrate, (unsigned char *) &writeint);
        fwrite(&writeint, sizeof(writeint), 1, ctx->indxfp);
    
        /* | avg_bitrate | */
        InsertI4(ctx->stats.avg_bitrate, (unsigned char *) &writeint);
        fwrite(&writeint, sizeof(writeint), 1, ctx->indxfp);
    
        /* | file_size | */
        InsertI4(ctx->stats.file_size, (unsigned char *) &writeint);
        fwrite(&writeint, sizeof(writeint), 1, ctx->indxfp);
    
        /* | num_frames | */
        InsertI4(ctx->stats.num_frames, (unsigned char *) &writeint);
        fwrite(&writeint, sizeof(writeint), 1, ctx->indxfp);
    
        /* | len_sec | */
        InsertI4(ctx->mpegtval.units, (unsigned char *) &writeint);
        fwrite(&writeint, sizeof(writeint), 1, ctx->indxfp);
    
        /* | len_mec | */
        InsertI4(ctx->mpegtval.usec, (unsigned char *) &writeint);
        fwrite(&writeint, sizeof(writeint), 1, ctx->indxfp);
    
        /* |mpeg_version_index| */
        InsertI4(ctx->stats.mpeg_version_index, (unsigned char *) &writeint);
        fwrite(&writeint, sizeof(writeint), 1, ctx->indxfp);
    
        /* |mpeg_layer| */
        InsertI4(ctx->stats.mpeg_layer, (unsigned char *) &writeint);
        fwrite(&writeint, sizeof(writeint), 1, ctx->indxfp);
    
        for (indx=0; indx<16; indx++) {
            InsertI4(ctx->stats.bitrate_frame_cnt[indx],
                     (unsigned char *) &writeint);
            fwrite(&writeint, sizeof(writeint), 1, ctx->indxfp);
        }
        fseek(ctx->indxfp, 0, SEEK_END);
    }
    
    return eof;
}



int mp3edit_indexfile_get_stats(void *ifctx)
{
    playctx          *ctx;
    int32_t          readint;
    int              sts;
    int              indx;
    int              version;

    ctx = (playctx *) ifctx;
    if (!ctx || !ctx->indxfp) {
        return -1;
    }
    fseek(ctx->indxfp, 0, SEEK_SET);

    sts = fread(&version, sizeof(version), 1, ctx->indxfp);
    if (sts != 1) {
        return -1;
    }
    if (version == 0) {
        /* Looking for version 3 index */
        return -1;
    }
    /* Found at least version 2 index file */
    sts = fread(&version, sizeof(version), 1, ctx->indxfp);
    if (sts != 1) {
        return -1;
    }
    version = ExtractI4((unsigned char *) &version);
    if (version < 3) {
        /* Looking for at least version 3 index */
        return -1;
    }

    /* | min_bitrate | */
    fread(&readint, sizeof(readint), 1, ctx->indxfp);
    ctx->stats.min_bitrate = ExtractI4((unsigned char *) &readint);
    
    /* | max_bitrate | */
    fread(&readint, sizeof(readint), 1, ctx->indxfp);
    ctx->stats.max_bitrate = ExtractI4((unsigned char *) &readint);

    /* | avg_bitrate | */
    fread(&readint, sizeof(readint), 1, ctx->indxfp);
    ctx->stats.avg_bitrate = ExtractI4((unsigned char *) &readint);

    /* | file_size | */
    fread(&readint, sizeof(readint), 1, ctx->indxfp);
    ctx->stats.file_size = ExtractI4((unsigned char *) &readint);

    /* | num_frames | */
    fread(&readint, sizeof(readint), 1, ctx->indxfp);
    ctx->stats.num_frames = ExtractI4((unsigned char *) &readint);

    /* | len_sec | */
    fread(&readint, sizeof(readint), 1, ctx->indxfp);
    ctx->mpegtval.units = ExtractI4((unsigned char *) &readint);

    /* | len_mec | */
    fread(&readint, sizeof(readint), 1, ctx->indxfp);
    ctx->mpegtval.usec = ExtractI4((unsigned char *) &readint);

    /* | mpeg_version_index | */
    fread(&readint, sizeof(readint), 1, ctx->indxfp);
    ctx->stats.mpeg_version_index = ExtractI4((unsigned char *) &readint);

    /* | mpeg_layer | */
    fread(&readint, sizeof(readint), 1, ctx->indxfp);
    ctx->stats.mpeg_layer = ExtractI4((unsigned char *) &readint);

    for (indx=0; indx<16; indx++) {
        fread(&readint, sizeof(readint), 1, ctx->indxfp);
        ctx->stats.bitrate_frame_cnt[indx] =
            ExtractI4((unsigned char *) &readint);
    }
    return 0;
}





long mp3edit_get_size_from_index(FILE *indxfp, long *sec, long *usec)
{
    int sts;
    long fsize;
    uint32_t readint;
    long tmp_sec;
    long tmp_usec;
    long version = 0;
    long offset;
   
    sts = fseek(indxfp, 0, SEEK_SET);
    if (sts == -1) {
        return -1;
    }
    if (fread(&readint, sizeof(readint), 1, indxfp) != 1) {
        return -1;
    }
    if (readint == ~0) {
        if (fread(&readint, sizeof(readint), 1, indxfp) != 1) {
            return -1;
        }
        version = ExtractI4((unsigned char *) &readint);
    }

    /*
     * Cannot accurately obtain file size from version 0 file, 
     * so return error.
     */
    if (version == 0) {
        return -1;
    }

    sts = fseek(indxfp, -8, SEEK_END);
    if (sts == -1) {
        return -1;
    }

    /*
     * File size in seconds is obtained by dividing the current file position
     * by 8. Each entry contains 2 integers, one for file byte offset,
     * the other for usec residual.  
     * For version 3 index, must subtract 116 bytes from the offset; 100 for
     * the file statistics information stored in the header, 8 for the
     * index version information, and 8 for the 0 second entry.
     * For version 2 index, must subtract 16 bytes from the offset;
     * 8 for the index version, and 8 for the 0 second entry.
     */
    offset = ftell(indxfp);
    if (version == 3) {
        offset -= 8 + 8 + 100;
    }
    else {
        offset -= 8 + 8;
    }
    tmp_sec = offset / 8;
     
    if (fread(&readint, sizeof(readint), 1, indxfp) != 1) {
        return -1;
    }
    fsize = ExtractI4((unsigned char *) &readint);

    if (fread(&readint, sizeof(readint), 1, indxfp) != 1) {
        return -1;
    }
    tmp_usec = ExtractI4((unsigned char *) &readint) & (~(1U<<31));
    *sec = tmp_sec;
    *usec = tmp_usec;

    return fsize;
}


/*
 * Given the existence of an mp3 file, and the corresponding 
 * index file, determine if the index file actually belongs
 * to the mp3 file.  This is determined a number of ways.
 * 1) Comparing the mp3 file size stored in the index file
 * with the actual file size of the mp3 file
 * 2) Pick some random time offsets from the index file and
 * see if they correspond to actual MPEG frame header locations.
 * 
 * Return TRUE if index file corresponds to MP3 file, FALSE otherwise.
 * Return -1 if index file is version 0 file.
 */
int mp3edit_isvalid_index(FILE *mpegfp, FILE *indxfp)
{
    long mpeg_size;
    long indx_mpeg_size;
    long sec, usec;
    char buf[3];

    if (!mpegfp || !indxfp) {
        return 0;
    }
    
    if (fseek(mpegfp, 0, SEEK_END) == -1) {
        return 0;
    }
    mpeg_size = ftell(mpegfp);

    indx_mpeg_size = mp3edit_get_size_from_index(indxfp, &sec, &usec);
    if (indx_mpeg_size == -1) {
        return -1;
    }

    /*
     * Check for ID3 tag at EOF when sizes differ by 128 bytes.
     */
    if (indx_mpeg_size + 128 == mpeg_size) {
        if (fseek(mpegfp, -128, SEEK_END) == 0) {
            if (fread(buf, sizeof(buf), 1, mpegfp) == 1) {
                if (!strncmp(buf, "TAG", 3)) {
                    indx_mpeg_size += 128;
                }
            }
        }
    }

    if (fseek(mpegfp, 0, SEEK_SET) == -1) {
        return 0;
    }
    return mpeg_size == indx_mpeg_size;
}


/*
 * Split a path into directory and filename component.  This function
 * understands relative, full path, and Windows drive letter specifications.
 * There are four important cases to be considered:
 *      input path       returned dir returned file
 *      ----------       ------------ -------------
 *   1) file             dir=./       file=file
 *   2) /dir/file        dir=/dir     file=file
 *   3) d:file           dir=d:       file=file
 *   4) d:/dir/file      dir=dir:/dir file=file
 *
 *   Returns pointer to directory, and file, or NULL on failure.
 */
/*
 * Function renamed: 
 * parse_pathname() -> mpgedit_pathname_parse()
 */
char *mpgedit_pathname_parse(const char *path,
                             char **ret_filebase, char **ret_ext)
{
    char *tmp_path;
    char *cp;
    char *dir  = NULL;
    char *file = NULL;
    char *ext  = NULL;

    tmp_path = strdup(path);
    if (!tmp_path) {
        goto clean_exit;
    }

    /* Convert all \ to / in path */
    for (cp = tmp_path; *cp; cp++) {
        if (*cp == '\\') {
            *cp = '/';
        }
    }

    cp = strrchr(tmp_path, '/');
    if (cp) {
        /*
         * Cases 2 and 4 above
         */
        *cp++ = '\0';
        dir = strdup(tmp_path);
        file = strdup(cp);
    }
    else if (isalpha(tmp_path[0]) && tmp_path[1] == ':') {
        /*
         * Case 3 above
         */
        dir = calloc(1, 3);
        if (!dir) {
            goto clean_exit;
        }
        dir[0] = tmp_path[0];
        dir[1] = ':';
        file = strdup(tmp_path+2);
    }
    else {
        /*
         * Case 1 above
         */
        dir = calloc(1, 3);
        if (!dir) {
            goto clean_exit;
        }
        strcpy(dir, ".");
        file = strdup(path);
    }
    cp = strrchr(file, '.');
    if (cp) {
        *cp++ = '\0';
        ext = strdup(cp);
    }
    if (!ext) {
        ext = strdup("");
    }

clean_exit:
    if (tmp_path) {
        free(tmp_path);
    }
    if (!dir || !file) {
        if (dir) {
            free(dir);
            dir = NULL;
        }
        if (file) {
            free(file);
            file = NULL;
        }
    }
    else {
        *ret_filebase = file;
        *ret_ext      = ext;
    }
    return dir;
}


/*
 * Given an input path to an mp3 file, return the path to the corresponding
 * index file.  One rule applies: when the directory containing the file
 * is read-only, the returned index path is in the local directory.
 */
/*
 * Function renamed: 
 * build_index_filename_2() -> mpgedit_index_build_filename_2()
 */
char *mpgedit_index_build_filename_2(char *path, char *indxext)
{
    char *dir        = NULL;
    char *filebase   = NULL;
    char *ext        = NULL;
    char *defext     = ".idx";
    char *index_file = NULL;
    int  len;

    dir = mpgedit_pathname_parse(path, &filebase, &ext);
    if (!dir) {
        goto clean_exit;
    }
    if (indxext) {
        defext = indxext;
    }

    /*
     * Is the directory read-only?
     */
    if (access(dir, W_OK) == -1) {
        free(dir);
        dir = strdup(".");
        if (!dir) {
            goto clean_exit;
        }

        /*
         * Is the directory still read-only? If so, fatal error, because
         * cannot write to the local directory.
         */
        if (access(dir, W_OK) == -1) {
            goto clean_exit;
        }
    }

    /*
     * Build the index file name. Length is dir + / + filebase + .idx + \0
     */
    len = strlen(dir) + 1+ strlen(filebase) + strlen(defext) + 1;
    index_file = calloc(1, len);
    if (!index_file) {
        goto clean_exit;
    }
    strcpy(index_file, dir);
    if (strlen(dir) != 2 || dir[1] != ':') {
        strcat(index_file, "/");
    }
    strcat(index_file, filebase);
    strcat(index_file, defext);
    
clean_exit:
    if (dir) {
        free(dir);
    }
    if (filebase) {
        free(filebase);
    }
    if (ext) {
        free(ext);
    }
    return index_file;
}


/*
 * Needed to preserve original function signature.
 */
/*
 * Function renamed: 
 * build_index_filename() -> mpgedit_index_filename()
 */
char *mpgedit_index_build_filename(char *path)
{
    return mpgedit_index_build_filename_2(path, NULL);
}


/*
 * Opens the index file corresponding to the file named by "mpeg_filename".
 * Returns a file pointer to the index file when it exists.  When
 * the index file exists, but does not pass the validity test, the
 * file is removed, and NULL is returned.
 */
/*
 * Function renamed: 
 * init_timeindex_file() -> mpgedit_indexfile_init()
 */
FILE *mpgedit_indexfile_init(FILE *fp,
                             char *mpeg_filename,
                             char **rindx_filename)
{
    FILE *indxfp = NULL;
    int  sts;
    char *indx_filename;

    if (!fp || !mpeg_filename) {
        return NULL;
    }

    indx_filename = mpgedit_index_build_filename(mpeg_filename);
    if (!indx_filename) {
        return NULL;
 
    }

    /*
     * Test existing index file for validity.  If it is deemed
     * incorrect remove the file, so it will be created anew.
     */
    indxfp = fopen(indx_filename, "r+b");
    if (!indxfp) {
        goto clean_exit;
    }

    sts = mp3edit_isvalid_index(fp, indxfp);
    if (sts != 1) {
        /*
         * Close file before removing it.  Does not matter on
         * UNIX, but on Win32, remove fails when the file
         * is still open.  This causes all sorts of interesting
         * subsequent failures to occur, because this bogus index
         * file persists, and is used.  This showed up in the
         * regression test suite testing the -X2 option.
         */
        fclose(indxfp);
        indxfp = NULL;
        remove(indx_filename);
        goto clean_exit;
    }

    rewind(indxfp);

clean_exit:
    if (rindx_filename) {
        *rindx_filename = indx_filename;
    }
    else {
        free(indx_filename);
    }
    return indxfp;
}



FILE *mpgedit_indexfile_open(char *name, char *mode)
{
    FILE *fp = NULL;

    if (name && mode) {
        fp = fopen(name, mode);
    }
    return fp;
}


void mpgedit_indexfile_close(FILE *fp)
{
    if (fp) {
        fclose(fp);
    }
}

static char *validate_outfile_status(int status, char *edit_filename)
{
    char *errstr = NULL;

    /*
     *  50 chars is approximately the length of additional text added below.
     */
    if (status) {
        errstr = (char *) malloc(strlen(edit_filename) + 50);
        if (!errstr) {
            return NULL;
        }
        if (status == VALIDATE_OUTFILE_FILE_EXIST) {
            sprintf(errstr, "Output file '%s' already exists",
                    edit_filename);
        }
        else if (status == VALIDATE_OUTFILE_FILE_NOWRITE){
            sprintf(errstr, "Output file '%s' is not writable for append",
                    edit_filename);
        }
        else if (status == VALIDATE_OUTFILE_NOFILE) {
            sprintf(errstr, "No file was specified");
        }
        else if (status == VALIDATE_OUTFILE_DIR_NOEXIST) {
            sprintf(errstr, "Directory in path does not exist '%s'",
                    edit_filename);
        }
        else if (status == VALIDATE_OUTFILE_DIR_NOWRITE) {
            sprintf(errstr, "Directory in path is not writable '%s'",
                    edit_filename);
        }
        else {
            sprintf(errstr, "Error using output file '%s'",
                    edit_filename);
        }
    }
    return errstr;
}

/*
 * Returns 0 if named file is ok
 *         1 if file already exists
 *         2 if append has been specified, and file is not writable
 *         3 if filename has not been specified
 */
int validate_outfile(char *filename, int append, char **errstr)
{
    int  status = VALIDATE_OUTFILE_OK;
    char *cp;
    char *newname = NULL;


    if (!filename) {
        status = VALIDATE_OUTFILE_NOFILE;
        goto clean_exit;
    }

    /*
     * Check if a directory path was specified.  If so, determine
     * if the specified directory is writable first.
     */
    cp = strrchr(filename, '/');
    if (cp) {
        newname = strdup(filename);
        if (newname) {
            cp = strrchr(newname, '/');
            *cp = '\0';
            if (access(newname, F_OK) != 0) {
                status = VALIDATE_OUTFILE_DIR_NOEXIST;
                goto clean_exit;
            }
            if (access(newname, W_OK) != 0) {
                status = VALIDATE_OUTFILE_DIR_NOWRITE;
                goto clean_exit;
            }
        }
    }
    if (append) {
        if (access(filename, F_OK) == 0) {
            if (access(filename, W_OK) != 0) {
                status = VALIDATE_OUTFILE_FILE_NOWRITE;
                goto clean_exit;
            }
        }
    }
    else {
        if (access(filename, F_OK) == 0) {
            status = VALIDATE_OUTFILE_FILE_EXIST;
            goto clean_exit;
        }
    }

clean_exit:
    if (newname) {
        free(newname);
    }
    if (errstr) {
        *errstr = validate_outfile_status(status, filename);
    }
    return status;
}

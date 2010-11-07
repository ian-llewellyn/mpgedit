/*
 * mpgedit application code
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
static char SccsId[] = "$Id: mp3_header.c,v 1.76.2.2 2009/04/02 03:14:02 number6 Exp $";
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include "portability.h"
#include "header.h"
#include "mp3time.h"
#include "mpegindx.h"
#include "xing_header.h"
#include "mpegfio.h"
#include "mp3_header.h"
#include "mpegcurses.h"
#include "decoder.h"
#include "plugin.h"
#include "version.h"
#include "editif.h"
#include "playif.h"
#include "volumeif.h"
#include "mpegstat.h"
#include "wav16.h"
#include "pcmlevel.h"
#include "segment.h"


typedef struct _mp3edit_play_cb_struct
{
    char  *filename;
    mpegfio_iocallbacks *ttyio;
    long  esec;
    long  emsec;
    long  tlast;
    long  frame_count;
    int   verbose;
    void *play_ctx;
    int   once;
#if 1
    int   stereo;
    int   silence_threshold;
    int   silence_repeat;
    int   silence_cnt;
    int   silence_print;

    long  silence_ssec;
    long  silence_smsec;

    mpgedit_pcmfile_t *pcmfp;
    char  *pcmlevelname;
    int   pcmavg1;
    int   pcmavg2;
    int   pcmavg1cnt;
    int   pcmavg2cnt;
    int   pcmmax;
    int   pcmmin;
    int   pcmmax_last;
#endif
} mp3edit_play_cb_struct;


static char *g_progname;

/* Leading plus forces POSIX processing of argument list */
#define OPTARGS "+hHf:vpEe:X::o:csVl:Ld:D::I"


void usage(int longhelp)
{
    fprintf(stderr, "usage: %s %s\n", 
            g_progname, "-V | -h | -H | [-c [input_file [input_file2] ... ]]");
    fprintf(stderr, "       %s %s%s%s\n", g_progname,
         "[-l left[:right]] [-L] [-p] [-d N] [-s] [-v[v[v[v[v]]]]] ",
         "[-X[0|1|2]] [-o [+]outfile_name] ",
         "[-e [begint[-[endt]]]] [-E] [-I] [-f] input_file");
    fprintf(stderr, "       %s %s\n", g_progname,
         "[-D] [-DbN] [-DcN] [-DmN] [-Ds] [-d N] input_file [input_file2 ...]");
    if (!longhelp) {
        exit(1);
    }

    if (longhelp == 2) {
        fprintf(stderr, 
"Where\n "
"  -c [input file [input_file2] ...]\n"
"      Curses edit mode.  Enter a full screen, interactive edit session\n"
"      to playback input files and set begin and end times for each\n"
"      edit.  This mode is the default action when no arguments\n"
"      are supplied.\n\n"
"  -d N\n"
"      Specifies playback or decoding decimation level.  When used with\n"
"      playback, every Nth frame is played. When used with -D, the default\n"
"      decimation level of 9 is replaced with the value specified.  There is\n"
"      a serious performance penalty for using a smaller decimation level\n"
"      when decoding (-D option), and rarely is the increased silence \n"
"      detection resolution required.\n\n"
"  -D  Decode input files and computes average volume for every decoded\n"
"      frame.  The amplitude values are stored in a file named 'input.lvl.\n"
"      Once decoded, segment boundaries separated by a minimum amplitude\n"
"      threshold are detected, storing the result of that analysis in the\n"
"      file 'levels_session'. The default decimation level used by this\n"
"      operation is 9, or about 3 frames per second. This default is changed\n"
"      with the -d option.\n\n"
"  -DbN\n"
"      Same as -D, but override the SILENCE_THRESHOLD parameter. The default\n"
"      value is 30, setting the silence threshold to -30db below the average\n"
"      input file amplitude.  Lower values detect segment boundaries\n"
"      containing louder content.  Valid values are 6-96.\n\n"
"  -DcN\n"
"      Same as -D, but override the SILENCE_REPEAT parameter, the number\n"
"      consecutive decoded samples below the silence threshold that must\n"
"      occur before a segment boundary is detected.  The default value is 3,\n"
"      or about one second of silence when the default decimation level (-d\n"
"      option) of 9 is used.\n\n"
"  -DmN\n"
"      Same as -D, but override the MINIMUM_TIME parameter, the number of\n"
"      seconds that must elapse once a segment boundary has been detected\n"
"      before another boundary can be detected.  The default value is 90\n"
"      seconds.\n\n"
"  -Ds\n"
"      Decode the input file, writing the output to standard output.\n\n"
"  -estart[-[end]] \n"
"      Perform copy/paste operation on input files. The input MP3\n"
"      file may be encoded as either CBR or VBR.  For quick and accurate\n"
"      random access into VBR encoded files, an external time index file\n"
"      is created.  The index file uses the basename of the input file,\n"
"      with the .idx extension.\n"
"      \n"
"      The output files created by this option are located in the\n"
"      same directory as the input MP3 file.  When the -o option is\n"
"      not used, the copy/paste data is written to a file\n"
"      with the same basename as the input file, with the '_nn.mp3'\n"
"      extension, where nn is an integer value. When a file with\n"
"      basename_nn.mp3 already exists, the integer extension is incremented\n"
"      until the filename is unique.  The input MP3 file is never\n"
"      overwritten by this option.\n"
"      \n"
"      When just a begin time is specified, the copy/paste operation is\n"
"      performed from that begin time through the end of file. When just\n"
"      an end time is specified, the copy/paste operation is performed\n"
"      from the beginning of the file through the specified end time.\n"
"      When both begin and end times are specified, the portion of the\n"
"      file bracketed by these times is copy/pasted.  Begin and end times\n"
"      are specified in seconds, or MM:SS.mmm, where MM are minutes, SS\n"
"      are seconds, and mmm are milliseconds.  Although any millisecond\n"
"      value may be specified, it will always be rounded off to the next\n"
"      frame boundary, approximately 26ms.\n"
"      \n"
"      Multiple -e options are allowed. Each -e represents a separate edit\n"
"      that will be performed on the named input file that follows the last\n"
"      -f option.\n\n"
"  -E  Suppress performing edit when -e option is used.  Useful when both\n"
"      -e and -v options are used to generate verbose output for a portion\n"
"      of an input file, while performing the edit is not desired.\n\n"
"  -I  Suppress generation of index file before performing edit.  Warning: \n"
"      use of this option can adversely affect edit performance.\n\n"
"  -f  input file name\n"
"      Multiple -f options are allowed.  At least one -e option must be\n"
"      preceed each -f option.\n\n"
"  -o  output file\n"
"      Prefixing a + to filename causes output from edit to be appended\n"
"      to the output file.  XING headers are automatically fixed up when\n"
"      using this option.\n\n"
"  -h  command usage summary\n\n"
"  -H  verbose usage information; this help screen\n\n"
"  -l  left[:right]\n"
"      Set the current playback device volume level to a value between 0 and\n"
"      100.  Specifying only a left value sets both left and right channels\n"
"      to that value.\n\n"
"  -L  Display the current playback device volume level.\n\n"
"  -p  play MPEG file using external player\n\n"
"  -s  silent operation\n"
"      All of the normal informational messages are suppressed during\n"
"      indexing and editing.\n\n"
"  -v  verbose output; adding more -v increases verbosity.\n"
"      Up to -vvvvv is allowed. -v displays the frame count and file\n"
"      offset at one second increments. -vv adds to -v this information\n"
"      for each frame: time offset, bitrate, frame size, frame count,\n"
"      frame position.  -vvv adds to -vv and displays a detailed breakdown\n"
"      of each MPEG frame header. -vvvv adds to -vv an md5 checksum of every\n"
"      frame. -vvvvv adds to -vvv an md5 checksum of every frame.\n\n"
"  -V  display mpgedit version and build number\n\n"
"  -X[0|1|2]\n"
"      Manipulate XING header. 0 omit XING header prefix, allowing output\n"
"      to be appended to the end of a previous edit. 1 fixes up XING header\n"
"      after catenating edits. This option is largely obsolete because of '+'\n"
"      option to -o. The -X2 option generates a new file, containing the\n"
"      XING header prefix, followed by the contents of the input file.\n"
"      When the -o option is not supplied, the new file basename_newxing_nn.mp3\n"
"      is generated.\n"
);
    }
    else {
        fprintf(stderr, "\n    For detailed mpgedit usage, use -H\n");
    }
    exit(1);
}




/*
 * Support function for mp3edit_find_free_slot().
 */
static int mp3edit_file_exists(char *base, const char *ext, char *offset, int n)
{
    sprintf(offset, "_%d.%s", n, ext);
    return access(base, F_OK) == 0;
}


static char *mp3edit_make_cached_slot_name(const char *base, const char *ext)
{
    char *cp;
    char *tp;

    cp = (char *) malloc(strlen(base)+strlen(ext)+5);
    if (!cp) {
        return NULL;
    }
    
    /* 
     * Transform path: path/file -> path/.file_#
     *                      file -> .file_#
     */
    cp[0] = '\0';
    tp = strrchr(base, '/');
    if (tp) {
        strncat(cp, base, tp-base+1);
        strcat(cp, ".");
        strcat(cp, tp+1);
    }
    else {
        strcpy(cp, ".");
        strcat(cp, base);
    }

    strcat(cp, ".");
    strcat(cp, ext);
    strcat(cp, "_#");
    return cp;
}


static int mp3edit_find_cached_free_slot(const char *name_cursor,
                                         const char *base,
                                         const char *ext,
                                         int *sequence)
{
    FILE *fp = NULL;
    char buf[1024];
    char *cp;
    char *endcp;
    int  n;
    int  rsts = 0;

    fp = fopen(name_cursor, "r+");
    if (!fp) {
        rsts = 1;
        goto clean_exit;
    }
    cp = fgets(buf, sizeof(buf)-1, fp);
    if (!cp) {
        rsts = 1;
        goto clean_exit;
    }

    /* Stomp trailing '\n' */
    cp[strlen(cp)-1] = '\0';

    /* Check that cached file exists.  If not, out of sync */
    if (access(cp, F_OK) == -1) {
        unlink(name_cursor);
        rsts = 1;
        goto clean_exit;
    }

    cp = strrchr(buf, '.');
    if (cp) {
        cp = strrchr(buf, '_');
    }
    if (!cp) {
        rsts = 1;
        goto clean_exit;
    }

    /* Hopefully, cp is now positioned at _NNN.mp3 in cached name */
    cp++;
    n = strtol(cp, &endcp, 10);
    if (*endcp != '.' || strcmp(endcp+1, ext)) {
        rsts = 1;
        goto clean_exit;
    }
    n++;
    sprintf(cp, "%d.%s", n, ext);

    
    /*
     * Check that next sequence numberef file exists.  If it does, out of sync
     */
    if (access(buf, F_OK) == 0) {
        unlink(name_cursor);
    }
    else {
        fseek(fp, 0, SEEK_SET);
        fprintf(fp, "%s\n", buf);

        /* Return next sequence number */
        *sequence = n;
    }

clean_exit:
    if (fp) {
        fclose(fp);
    }

    
    return rsts;
}


static int mp3edit_write_cached_slot_name(const char *name_cursor, char *base)
{
    int rsts = 0;
    FILE *fp;

    fp = fopen(name_cursor, "wb");
    if (!fp) {
        rsts = 1;
    }
    else {
        fprintf(fp, "%s\n", base);
        fclose(fp);
    }
    return rsts;
}


/*
 * Find the next available output file.   Given a file with the name
 * name.mp3, this function builds the name name_NNN.mp3, where NNN
 * is an integer number.  This function finds the next unused file
 * name in the NNN sequence. 
 *
 * The search for the next available file is implemented as a binary
 * search.  The search starts by finding the first unused filename, by
 * incrementing NNN by powers of 2.  Once this high number is found,
 * then the binary search begins.
 *
 * This function modifies the space pointed at by base.  This
 * function is an internal helper routine for build_edit_filename();
 *
 * Support function for build_edit_filename().
 */
static char *mp3edit_find_free_slot(char *in_base, const char *ext)
{
    int err  = 0;
    int low  = 1;
    int high = 1;
    int test = 0;
    int rsts;
    char *offset;
    char *base       = NULL;
    char *cache_file = NULL;

    /*
     * Convert /path/prefix.mp3 -> /path/prefix_N.mp3.
     * New size is length of path + prefix + "_" (1) + max integer
     * size (11) + ".ext" + null byte (1).
     */
    base = (char *) calloc(1, strlen(in_base) + 1 + 11 + 1 + strlen(ext) + 1);
    if (!base) {
        return NULL;
    }
    strcpy(base, in_base);
    offset = base + strlen(base);

    /*
     * Search for first available unused slot
     */
    cache_file = mp3edit_make_cached_slot_name(base, ext);
    if (!cache_file) {
        err = 1;
        goto clean_exit;
    }

    rsts = mp3edit_find_cached_free_slot(cache_file, base, ext, &test);
    if (rsts == 0) {
       /* Format the return file name using the cached slot number */
       sprintf(offset, "_%d.%s", test, ext);
       goto clean_exit;
    }

    rsts = mp3edit_file_exists(base, ext, offset, high);
    while (rsts) {
        low = high;
        high *= 2;
        rsts = mp3edit_file_exists(base, ext, offset, high);
    }

    /* found slot if first numbered file does not exit */
    if (low == high) {
        goto clean_exit;
    }

    /* Perform binary search for next available slot */
    do {
        test = low + ((high - low) / 2);
        rsts = mp3edit_file_exists(base, ext, offset, test);
        if (rsts) {
            low = test;
        }
        else {
            high = test;
        }
    } while ((low+1) != high);

    /* Format the file name using the high value */
    mp3edit_file_exists(base, ext, offset, high);
    mp3edit_write_cached_slot_name(cache_file, base);
    free(cache_file);

clean_exit:
    if (err) {
        if (base) {
            free(base);
        }
    }
    return base;
}


/*
 * Given an input path to an mp3 file, return the path to the corresponding
 * edit output file.  One rule applies: when the directory containing the file
 * is read-only, the returned output file is in the local directory.
 * This function builds a unique output file name, by adding the suffix
 * _NNN between the filebase and the file extension.  Given /path/file.mp3,
 * the output file name /path/file_1.mp3 is created.  Should that file exist,
 * the integer suffix is incremented until the file does not exist.
 *
 * API function called by main().
 */
static char *build_edit_filename(const char *infile_name, const char *suffix)
{
    char *edit_filename = NULL;
    char *dirp      = NULL;
    char *filebasep = NULL;
    char *extp      = NULL;
    char *dir       = NULL;
    char *filebase  = NULL;
    char *ext       = NULL;
    int  len;
    char *ret_filename = NULL;

    dirp = mpgedit_pathname_parse(infile_name, &filebasep, &extp);
    if (!dirp) {
        goto clean_exit;
    }

    /*
     * Beware of memory allocated within mpgedit shared library.  Duplicate
     * all of these strings here, and free them with mpgedit_free() now.
     */
    if (dirp) {
        dir = strdup(dirp);
        mpgedit_free(dirp);
    }
    if (filebasep) {
        filebase = strdup(filebasep);
        mpgedit_free(filebasep);
    }
    if (extp) {
        ext = strdup(extp);
        mpgedit_free(extp);
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
     * Convert /path/prefix.mp3 -> /path/prefix_N.mp3.
     * New size is length of path + prefix + "_" (1) + max integer
     * size (11) + ".ext" + null byte (1).
     */
    len = strlen(dir) + 1 + strlen(filebase) + 1 + 11 + 1 + strlen(ext) + 1;
    if (suffix) {
        len += strlen(suffix);
    }
    edit_filename = (char *) calloc(1, len);
    if (!edit_filename) {
        fprintf(stdout, "failed to alloc edit filename\n");
        goto clean_exit;
    }
    strcpy(edit_filename, dir);
    if (strlen(dir) != 2 || dir[1] != ':') {
        strcat(edit_filename, "/");
    }
    strcat(edit_filename, filebase);
    if (suffix) {
        strcat(edit_filename, suffix);
    }
    if (!ext || !*ext) {
        if (ext) {
            free(ext);
        }
        ext = strdup("mp3");
        if (!ext) {
            goto clean_exit;
        }
    }
    ret_filename = mp3edit_find_free_slot(edit_filename, ext);
    free(edit_filename);

clean_exit:
    if (!dir || !filebase || !ext) {
        if (ret_filename) {
            free(ret_filename);
            ret_filename = NULL;
        }
    }

    if (dir) {
        free(dir);
    }
    if (filebase) {
        free(filebase);
    }
    if (ext) {
        free(ext);
    }
    return ret_filename;
}


static void nullprintf(void *ctx, int y, int x, const char *fmt, ...)
{
}

static void myprintf(void *ctx, int y, int x, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    fflush(stdout);
    va_end(ap);
}


/*
 * Not an exhaustive validation of the provided time specification,
 * but a consistency check.
 */
static int validate_edit_timespec(char *str)
{
    /* Null pointer or null string not valid */
    if (!str || !*str) {
        return 0;
    }

    /* 12 || :34 check */
    if (isdigit(str[0]) || str[0] == ':') {
        return 1;
    }

    /*
     * Must look at character following - to prove the value is not
     * the next option on the command line; -, -1:34 and -34 are valid.
     */
    if (str[0] == '-') {
        if (str[1] != '\0' && str[1] != ':' && !isdigit(str[1])) {
            return 0;
        }
        return 1;
    }

    return 0;
}


static void mpgedit_version(void)
{
    printf("\n%s version %s %s\n",
           MPGEDIT_PRODUCT,
           MPGEDIT_VERSION,
           MPGEDIT_URL);
    printf("Build:          %s (%s)\n", MPGEDIT_BUILDNUM, MPGEDIT_BUILDOS);
    printf("Report bugs to: %s\n", MPGEDIT_MAILTO);
    exit(1);
}


static char *mp3edit_alloc_infile(char *name)
{
    char *filename;
    char *cp;

    filename = strdup(name);
    if (!filename) {
        return NULL;
    }

    /*
     * Make all \ directory separators a /, as much of the subdirectory
     * processing code assumes a /, and Win32 filesystem functions will
     * accept a /.
     */
    for (cp = filename; *cp; cp++) {
        if (*cp == '\\') {
            *cp = '/';
        }
    }
    return filename;
}


typedef struct _seektime_iocallbacks
{
    void *ctx;
    int  cb_called;
    long start_sec;
    long start_msec;
} seektime_iocallbacks;


/*
 * Used when seeking time with -I flag
 */
static int seek_callback(void *data, long sec, long msec)
{
    seektime_iocallbacks *ttyio = (seektime_iocallbacks *) data;

    if (!data) {
        return 1;
    }
    if (!ttyio->cb_called) {
        printf("Seeking to offset...\n");
        ttyio->cb_called = 1;
    }
    if (sec == -1 && msec == -1) {
        printf("\n");
        ttyio->cb_called = 0;
    }
    else {
        printf("\rSeeking to time: %ld:%02ld| %ld.000->"
               "%ld:%02ld| %ld.%03ld| Frame %-7ld\r",
               mpgedit_edit_sec(ttyio->ctx)/60,
               mpgedit_edit_sec(ttyio->ctx)%60,
               mpgedit_edit_sec(ttyio->ctx),
               sec/60,
               sec%60,
               sec,
               msec/1000,
               mpgedit_edit_index_frames(ttyio->ctx));
    }
    fflush(stdout);
    return 1;
}


static int index_callback(void *data, long sec, long msec,
                          long frame, char *indx_filename)
{
    mpegfio_iocallbacks *ttyio = (mpegfio_iocallbacks *) data;

    if (!data || !ttyio->printf) {
        return 1;
    }
    if (!ttyio->cb_called) {
        ttyio->printf(ttyio->ctx, 2, 0, 
                      "Creating index file %s\n", indx_filename);
        ttyio->cb_called = 1;
    }
    ttyio->printf(ttyio->ctx, 2, 0,
                  "\rElapsed time: %4d:%02d| %5d.%03ds|Frame: %-7d\r",
                  sec/60,
                  sec%60,
                  sec,
                  msec,
                  frame);
    return 1;
}


#if 1
/*
 * adam/TBD: Perhaps this function should be converted to a formatting
 * 2str function that lives in mpgeditlib.  That would allow more of
 * the internal library functions, like mpeg_time_gettime(), to not
 * be exported from the shared library.  This conversion is inevitable
 * when this verbose functionality is added to xmpgedit.
 */
#endif
static void mp3header_print_verbose(void *edit_ctx,
                                    int  verbose,
                                    int  *once,
                                    long *tprev)
{
    mpeg_header_data *stats_header;
    long             tnow = 0;
    mpeg_file_stats  *stats_ptr;
    mpeg_time        *file_time;
    long             sec;
    long             usec;
    long             frame_start = 0;
    char             *buf;

    buf = malloc(1024);
    if (!buf) {
        return;
    }
    if (!(*once)) {
        *once = 1;
        if (verbose > 0 &&
            mpgedit_edit_has_xing(edit_ctx))
        {
            printf("Found xing header\n");
            xingheader2str(mpgedit_edit_xing_header(edit_ctx), buf);
            printf("%s", buf);
        }
    }
    stats_header = mpgedit_edit_frame_header(edit_ctx);
    tnow         = mpgedit_edit_sec(edit_ctx);
    stats_ptr    = mpgedit_edit_stats_ctx(edit_ctx);
    file_time    = mpgedit_edit_total_length_time(edit_ctx);
    mpeg_time_gettime(file_time, &sec, &usec);

    if (verbose > 0) {
        if (verbose > 2) {
            printf("MPEG header ");
        }

        if (verbose > 1) {
            frame_start = mpgedit_edit_offset(edit_ctx) -
                              stats_header->frame_size;
            printf("t=%d.%-.3ds  br=%-3d  sz=%-4d "
                   "fr=%-6ld pos=%-10ld pos=0x%-8lx",
                   (int) sec, (int) (usec/1000),
                   stats_header->bit_rate,
                   stats_header->frame_size,
                   mpgedit_edit_frames(edit_ctx),
                   frame_start, frame_start);
            if (verbose > 3) {
                printf("md5sum=0x%02x%02x%02x%02x%02x%02x%02x%02x"
                                "%02x%02x%02x%02x%02x%02x%02x%02x",
                       stats_header->md5sum[0],  stats_header->md5sum[1],
                       stats_header->md5sum[2],  stats_header->md5sum[3],
                       stats_header->md5sum[4],  stats_header->md5sum[5],
                       stats_header->md5sum[6],  stats_header->md5sum[7],
                       stats_header->md5sum[8],  stats_header->md5sum[9],
                       stats_header->md5sum[10], stats_header->md5sum[11],
                       stats_header->md5sum[12], stats_header->md5sum[13],
                       stats_header->md5sum[14], stats_header->md5sum[15]);
            }
            printf("\n");
        }

        if (verbose==3 || verbose==5) {
            mpeg_header_values2str_3(stats_header, frame_start, buf);
            printf("%s", buf);
        }
        if (*tprev != tnow) {
            *tprev = tnow;
            printf("  *** t=%lds fr=%ld pos=%ld (0x%lx)\n",
                   tnow, 
                   stats_ptr->num_frames,
                   mpgedit_edit_offset(edit_ctx),
                   mpgedit_edit_offset(edit_ctx));
        }
    }
    else if (verbose != -1) {
        if (*tprev != tnow) {
            *tprev = tnow;
            printf("\rEdit time: %4ld:%02ld| %5ld.%03lds|Frame: %-7ld\r",
                   tnow/60,
                   tnow%60,
                   tnow,
                   usec/1000,
                   stats_ptr->num_frames
              );
            fflush(stdout);
        }
    }
    free(buf);
}


static int play_callback_func(void *data, long sec, long msec)
{
    int go = 1;

    mp3edit_play_cb_struct *ctx = (mp3edit_play_cb_struct *) data;

    if (sec > ctx->esec || (sec == ctx->esec && msec >= ctx->emsec)) {
        go = 0;
    }
    if (go) {
        mp3header_print_verbose(
            ctx->play_ctx,
            ctx->verbose,
            &ctx->once,
            &ctx->tlast);
    }

    return go;
}


static int play_decode_callback_func(void *data,
                                     unsigned char *pcmbuf, long pcmlen,
                                     long sec, long msec)
{
    int go = 1;
    int channel_mode;
    int pcmmax;

    mp3edit_play_cb_struct *ctx = (mp3edit_play_cb_struct *) data;
    mpegfio_iocallbacks *ttyio = ctx->ttyio;

    if (!ctx->once) {
        /*
         * All this to figure out if we are stereo or mono
         */
        ctx->once = 1;
        mpgedit_play_decode_frame_header_info(
            ctx->play_ctx, NULL, NULL, NULL, NULL, NULL, NULL,
            &channel_mode, NULL);
        ctx->stereo = (channel_mode != 3);
        mpgedit_pcmlevel_write_header(ctx->pcmfp, 1, 16, 22, 10);
        /*
         * placeholder entry, will backfill the correct values later
         */
        mpgedit_pcmlevel_write_average(ctx->pcmfp, 0, 0, 0);
    
    }
    if (sec > ctx->esec || (sec == ctx->esec && msec >= ctx->emsec)) {
        go = 0;
    }
    if (go) {
        /*
         * adam/TBD: This is a hack to work around a layer3 decoding 
         * problem.  Occasionally, a frame cannot backstep some bits,
         * resulting in a zero level frame. Substitute the last non-zero
         * level frame in  this case.
         */
        if (pcmlen == 0) {
#if 0
printf("WARNING: frame %ld is 0 length\n", 
mpgedit_edit_index_frames(ctx->play_ctx));
#endif
            pcmmax = ctx->pcmmax_last;
        }
        else {
            pcmmax = wav16_samples_max(pcmbuf, pcmlen, ctx->stereo, 0);
            ctx->pcmmax_last = pcmmax;
        }

        /*
         * Compute max/min frame level
         */
        if (pcmmax > ctx->pcmmax) {
            ctx->pcmmax = pcmmax;
        }
        if (pcmmax > 0 && pcmmax < ctx->pcmmin) {
            ctx->pcmmin = pcmmax;
        }

        /*
         * Average computation.
         */
        ctx->pcmavg1 += pcmmax;
        ctx->pcmavg1cnt++;
        if (ctx->pcmavg1cnt > 30000) {
            ctx->pcmavg2 += ctx->pcmavg1 / ctx->pcmavg1cnt;
            ctx->pcmavg2cnt++;
            ctx->pcmavg1    = 0;
            ctx->pcmavg1cnt = 0;
        }

        mpgedit_pcmlevel_write_entry(ctx->pcmfp, pcmmax, sec, msec);
#if 0
ttyio->printf(ttyio->ctx, 2, 0, " %p pcmavg=%6d %6ld:%02ld.%03ld\n", ctx->pcmfp, pcmmax, sec/60, sec%60, msec);
#endif
    }

    if (!ttyio->cb_called) {
        ttyio->printf(ttyio->ctx, 2, 0, 
                      "Creating audio levels file %s\n", ctx->pcmlevelname);
        ttyio->cb_called = 1;
    }
    ttyio->printf(ttyio->ctx, 2, 0,
                  "\rDecode time: %4d:%02d| %5d.%03ds|Frame: %-7d\r",
                  sec/60,
                  sec%60,
                  sec,
                  msec,
                  mpgedit_edit_index_frames(ctx->play_ctx));

    return go;
}


/*
 * Create index file for named mp3 file.  When not NULL, the function
 * pointer 'callback' is called for each second of file processed, passed
 * the user-provided 'callback_data', and the current file second/millisecond
 * offset, the current frame number and the name of the index file.
 * When not NULL, after the file is indexed, the file file statistics,
 * and total length are returned.
 */
int mp3header_index_create(char *filename, 
                           int (*callback)(void *,   /* callback_data */
                                           long,     /* sec */
                                           long,     /* msec */
                                           long,     /* frame_count */
                                           char *),  /* index filename */
                           void *callback_data,
                           mpeg_file_stats *rstats,
                           mpeg_time *rtime)
{
    char                *indx_filename = NULL;
    void                *ctx;
    long                sec;
    long                msec;
    long                tprev = -1;
    long                frame_count;
    int                 cbrsts = 0;
    int                 eof = 0;
    int                 rsts = 0;

    indx_filename = mpgedit_index_build_filename(filename);
    if (!indx_filename) {
        return 1;
    }

    ctx    = mpgedit_edit_index_new(filename, &rsts);
    /* Returns MPGEDIT_EDIT_INDEX_STS_INDEX_EXISTS when index already exists */
    if (rsts == MPGEDIT_EDIT_INDEX_STS_OK) {
        eof    = mpgedit_edit_index(ctx);
        cbrsts = 1;
    }
    while (cbrsts) {
        sec         = mpgedit_edit_index_sec(ctx);
        msec        = mpgedit_edit_index_msec(ctx);
        frame_count = mpgedit_edit_index_frames(ctx);
        if (callback && sec > tprev) {
            tprev = sec;
            cbrsts = callback(callback_data, sec,
                              msec, frame_count, indx_filename);
        }

        if (eof) {
            cbrsts = 0;
        }

        if (cbrsts) {
            eof = mpgedit_edit_index(ctx);
        }
    }

    if (ctx) {
        if (rstats) {
            *rstats = *mpgedit_edit_stats_ctx(ctx);
        }
        if (rtime) {
            *rtime = *mpgedit_edit_total_length_time(ctx);
        }
        mpgedit_edit_index_free(ctx);
    }
    else {
        rsts = 1;
    }

    mpgedit_free(indx_filename);
    return rsts;
}


static void play_files(editspec_t *next_editspec, cmdflags *argvflags)
{
    int indx;
    void                   *play_ctx = NULL;
    mp3edit_play_cb_struct play_cb_data;
    int                    go;
    char                   *str;
    long                   sec;
    long                   usec;
    int                    ndx;
    int                    d_val = argvflags->d_val;
    seektime_iocallbacks   seekcb_ctx;
    unsigned long          flags = argvflags->I_flag?MPGEDIT_FLAGS_NO_INDEX:0;

    str = malloc(1024);
    if (!str) {
        return;
    }
    for (indx=0; indx<mpgedit_editspec_get_length(next_editspec); indx++) {
        /*
         * play files
         */
        play_ctx = mpgedit_play_init(mpgedit_editspec_get_file(
                                     next_editspec, indx), flags);
        if (!play_ctx) {
            printf("ERROR: Failed opening file '%s'\n\n",
                   mpgedit_editspec_get_file(next_editspec, indx));
            continue;
        }

        /*
         * Initialize mixer levels to specified volume level, or use default
         * value. On some platforms, the default value leaves the mixer level
         * at the current setting.
         */
        if (argvflags->l_flag) {
            mpgedit_play_volume_init(play_ctx,
                                     argvflags->vol_left, argvflags->vol_right);
        }
        else {
            mpgedit_play_volume_init(play_ctx,
                                     VOLUME_DEFAULT_VALUE,
                                     VOLUME_DEFAULT_VALUE);
        }

        if (argvflags->I_flag) {
            memset(&seekcb_ctx, 0, sizeof(seekcb_ctx));
            seekcb_ctx.ctx = play_ctx;
            mpgedit_edit_set_seek_callback(
                play_ctx, seek_callback, &seekcb_ctx);
        }

        mpeg_time_gettime(mpgedit_editspec_get_stime(next_editspec, indx),
                          &sec, &usec);
        mpgedit_play_seek_time(play_ctx, 
                               sec, usec/1000);
        memset(&play_cb_data, 0, sizeof(play_cb_data));
        mpeg_time_gettime(mpgedit_editspec_get_etime(next_editspec, indx),
                          &sec, &usec);
        play_cb_data.esec      = sec;
        play_cb_data.emsec     = usec/1000;
        play_cb_data.verbose   = argvflags->s_flag ? -1 :
                                 argvflags->v_flag;
        play_cb_data.play_ctx  = play_ctx;
        
        mpgedit_play_set_status_callback(play_ctx,
                                         play_callback_func,
                                         &play_cb_data);
        do {
            for (ndx=1; ndx<d_val; ndx++) {
                mpgedit_play_skip_frame(play_ctx);
            }
            go = mpgedit_play_frame(play_ctx);
        } while (go);
        mpgedit_edit_stats2str(play_ctx,
                               mpgedit_edit_total_length_time(play_ctx),
                               str);
        printf("%s", str);
          
        mpgedit_play_close(play_ctx);
    }
    free(str);
}


void _mpgedit_decode_file(char *fname,
                          long ssec, long susec,
                          long esec, long eusec,
                          cmdflags *argvflags,
                          mpegfio_iocallbacks *ttyio)
{
    void                   *play_ctx = NULL;
    mp3edit_play_cb_struct play_cb_data;
    int                    go;
    char                   *str;
    unsigned char          *pcmbuf;
    int                    pcmlen;
    int                    bsbytes;
    int                    ndx;
    int                    d_val = argvflags->d_val;
    char                   *pcmlevelname = NULL;
    int                    pcmavg;

    mpgedit_pcmfile_t      *pcmh;
    int                    pcm_version;
    int                    pcm_bits;
    int                    pcm_bitssec;
    int                    pcm_bitsmsec;
    int                    pcm_average = 0;
    int                    pcm_avgmax = 0;
    int                    pcm_avgmin = 0;

    memset(&play_cb_data, 0, sizeof(play_cb_data));

    /*
     * play files
     */
    play_ctx = mpgedit_play_init(fname, 0);
    if (!play_ctx) {
        return;
    }
    mpgedit_play_seek_time(play_ctx, ssec, susec/1000);
    play_cb_data.esec              = esec;
    play_cb_data.emsec             = eusec/1000;
    play_cb_data.verbose           = argvflags->s_flag ? -1 :
                                     argvflags->v_flag;
    play_cb_data.play_ctx          = play_ctx;
    play_cb_data.ttyio             = ttyio;
    play_cb_data.pcmmax            = -1;
    play_cb_data.pcmmin            = 100000;
    
    if (!argvflags->Ds_flag) {
        pcmlevelname = mpgedit_index_build_filename_2(fname, ".lvl");
        pcmh = mpgedit_pcmlevel_open(pcmlevelname, "rb");
        if (pcmh) {
            /*
             * Read header data down to average values.  File is incomplete
             * when average header values are all 0's.
             */
            if (mpgedit_pcmlevel_read_header(pcmh, &pcm_version,
                &pcm_bits, &pcm_bitssec, &pcm_bitsmsec))
            {
                if (!mpgedit_pcmlevel_read_average(pcmh, &pcm_average,
                    &pcm_avgmax, &pcm_avgmin))
                {
                    pcm_average = pcm_avgmax = pcm_avgmin = 0;
                }
            }
            else {
                pcm_average = pcm_avgmax = pcm_avgmin = 0;
            }
                                                                                                   
            if (pcm_average != 0 || pcm_avgmax != 0 || pcm_avgmin != 0) {
                mpgedit_pcmlevel_close(pcmh);
                return;
            }
        }
        play_cb_data.pcmfp        = mpgedit_pcmlevel_open(pcmlevelname, "wb");
        play_cb_data.pcmlevelname = pcmlevelname;

        mpgedit_play_decode_set_status_callback(
            play_ctx,
            play_decode_callback_func,
            &play_cb_data);
    }

    do {
        for (ndx=1; ndx<d_val; ndx++) {
            mpgedit_play_skip_frame(play_ctx);
        }
        go = mpgedit_play_decode_frame(
                 play_ctx, &pcmbuf, &pcmlen, &bsbytes);
        if (go && argvflags->Ds_flag) {
            int sts;
            sts = fwrite(pcmbuf, pcmlen, 1, stdout);
        }
    } while (go);

    if (!argvflags->Ds_flag) {
        /*
         * Write extra entry at end of lvl file with the time of the
         * last frame.
         */
         mpgedit_pcmlevel_write_entry(play_cb_data.pcmfp,
                                      play_cb_data.pcmmax_last, 
                                      mpgedit_play_current_sec(play_ctx), 
                                      mpgedit_play_current_msec(play_ctx));

        /*
         * Finalize average PCM level computation and write to
         * placeholder position in levels file.
         */
        if (play_cb_data.pcmavg1cnt) {
            play_cb_data.pcmavg2 +=
                play_cb_data.pcmavg1 / play_cb_data.pcmavg1cnt;
        }
        pcmavg                = play_cb_data.pcmavg2 /
                                (play_cb_data.pcmavg2cnt + 1);
        mpgedit_pcmlevel_seek(play_cb_data.pcmfp, 4);
        mpgedit_pcmlevel_write_average(play_cb_data.pcmfp, pcmavg,
                               play_cb_data.pcmmax, play_cb_data.pcmmin);
        mpgedit_pcmlevel_close(play_cb_data.pcmfp);
        mpgedit_free(pcmlevelname);
    }

    if (!argvflags->s_flag && !argvflags->Ds_flag) {
        str = malloc(1024);
        if (!str) {
            return;
        }
        mpgedit_edit_stats2str(play_ctx,
                               mpgedit_edit_total_length_time(play_ctx),
                               str);
        printf("%s", str);
        free(str);
    }
    mpgedit_play_close(play_ctx);
}


static void decode_files(editspec_t *next_editspec, cmdflags *argvflags,
                         mpegfio_iocallbacks *ttyio)
{
    long ssec;
    long susec;
    long esec;
    long eusec;
    char *fname;
    int indx;

    for (indx=0; indx<mpgedit_editspec_get_length(next_editspec); indx++) {
        fname = mpgedit_editspec_get_file(next_editspec, indx);
        mpeg_time_gettime(mpgedit_editspec_get_stime(next_editspec, indx),
                          &ssec, &susec);
        mpeg_time_gettime(mpgedit_editspec_get_etime(next_editspec, indx),
                          &esec, &eusec);
        _mpgedit_decode_file(fname, ssec, susec, esec, eusec, argvflags, ttyio);
    }
}


static void find_segment_boundaries(editspec_t *next_editspec,
                                    cmdflags *argvflags,
                                    mpgedit_pcmlevel_t *levelconf)
{
    int indx;
    int jndx;
    char *file;
    mpeg_time *segments;
    int       segmentslen;
    FILE      *fp;
    int       sts;

    unlink("levels_session");
    for (indx=0; indx<mpgedit_editspec_get_length(next_editspec); indx++) {
        /*
         * play files
         */
        file = mpgedit_editspec_get_file(next_editspec, indx);
        sts = mpgedit_segment_find(file, levelconf, &segments, &segmentslen);
        if (sts == 0) {
            fp = fopen("levels_session", "a"); 
            if (!fp) {
                fprintf(stderr, "Failed opening session file '%s'\n",
                        "levels_session");
                return;
            }
            for (jndx=1; jndx<segmentslen; jndx++) {
                fprintf(fp,
                        "%s%%%%%6ld:%02ld.%03ld%%%%%6ld:%02ld.%03ld\n",
                        file,
                        segments[jndx-1].units/60,
                        segments[jndx-1].units%60,
                        segments[jndx-1].usec/1000,
        
                        segments[jndx].units/60,
                        segments[jndx].units%60,
                        segments[jndx].usec/1000);
            }
            fclose(fp);
        }
    }
}


int read_mp3_header_file(char *file, int *channel_mode, int *sample_rate)
{
    unsigned char    buf[6];
    mpeg_header_data header;
    mpeg_file_iobuf  mpegiobuf;
    mpeg_header_data stats_header;
    int              eof;
    int              found;
    FILE             *infp;

    infp = mpeg_file_open(file, "rb");
    if (!infp) {
        return 1;
    }

    eof = 0;
    memset(&header, 0, sizeof(header));
    mpeg_file_iobuf_clear(&mpegiobuf);
    memset(&stats_header, 0, sizeof(stats_header));
    found = mpeg_file_next_frame_read(infp, &mpegiobuf, &stats_header, &eof);
    if (found && !eof) {
        memcpy(&buf, mpeg_file_iobuf_getptr(&mpegiobuf), 6);
        if (mpgedit_header_decode(buf, &header, MPGEDIT_ALLOW_MPEG1L1)) {
            *channel_mode = header.channel_mode;
            *sample_rate  = header.sample_rate;
        }
    }
    mpeg_file_close(infp);
    return 0;
}


int main(int argc, char *argv[])
{
    FILE             *editfp = NULL;
    int              ch;
    mpeg_time        mpegtval;
    char             *edit_filename = NULL;
    editspec_t       *curs_edits;
    int              status;
    mpegfio_iocallbacks ttyio;
    cmdflags         argvflags;
    char             *errstr;

    int              editing;
    void             *edit_ctx;
    void             *play_ctx = NULL;
    int              rsts = 0;
    int              edit_flags = 0;
    int              edit_indx;
    editspec_t       *next_editspec;
    int              next_editspec_len;
    int              outfile_indx;
    char             *edit_outfilename = NULL;
    char             *cp;
    long             tprev = 0;
    mpeg_file_stats  indx_stats;
    mpeg_file_stats  edit_stats;
    int              once = 0;
    int              indx;
    int              no_edits_msg = 0;
    int              edits_len;
    char             *str;
    mpgedit_pcmlevel_t *levelconf;
    int              intarg;
    seektime_iocallbacks seekcb_ctx;
    int              channel_mode;
    int              sample_rate;

    g_progname = (g_progname = strrchr(argv[0], '/')) ?
                     g_progname+1 : argv[0];

    memset(&argvflags, 0, sizeof(argvflags));
    memset(&ttyio, 0, sizeof(ttyio));
    levelconf = mpgedit_pcmlevel_new(NULL);
    mpgedit_pcmlevel_init_defaults(levelconf);

    /* 
     * More memory is allocated than needed here, but this saves
     * having to pre-count the total number of -e options specified.
     */
    argvflags.edits = mpgedit_editspec_init();
    if (!argvflags.edits) {
        perror("mpgedit main: mpgedit_editspec_init failed");
        return 1;
    }
    opterr = 0;
    while ((ch = getopt(argc, argv, OPTARGS)) != -1) {
        switch (ch) {
          case 'h':
            usage(1);
            break;

          case 'H':
            usage(2);
            break;

          case 'f':
            mpgedit_editspec_append(argvflags.edits,
                                    mp3edit_alloc_infile(optarg), NULL);
            break;

          case 'o':
            if (argvflags.out_filename) {
                fprintf(stderr, "Error: can only specify -o once\n");
                usage(0);
            }
            argvflags.out_filename = strdup(optarg);
            break;

          case 'p':
            argvflags.p_flag = 1;
            break;

          case 'v':
            argvflags.v_flag++;
            break;

          case 'e':
            argvflags.e_flag = 1;
            if (optarg) {
                if (!validate_edit_timespec(optarg)) {
                    fprintf(stderr, "%s '%s' %s\n",
                            "Error: -e argument",
                            optarg,
                            "is not a valid time specification");
                    usage(0);
                }
                mpgedit_editspec_append(argvflags.edits, NULL, optarg);
            }
            break;

          case 'E':
            argvflags.E_flag = 1;
            break;

          case 'X':
            /*
             * XING header processing. -X is same as -X0.  -X0 prevents
             * addition of XING header to edited cut. -X1 performs analysis of
             * edited cut and fixes up header. -X2 restores Xing header to
             * file, then updates it, ala -X1 analysis.
             */
            argvflags.X_flag = 1;
            if (optarg) {
                argvflags.X_args = strdup(optarg);
                argvflags.X_val  = atoi(optarg);
            }

            if (argvflags.X_val == 2) {
                /*
                 * X2 logic
                 */
                argvflags.e_flag = 1;
                mpgedit_editspec_append(
                    argvflags.edits,
                    mp3edit_alloc_infile(XING_HEADER_FILE),
                    "-");
            }
            break;

          case 's':
            argvflags.s_flag = 1;
            break;

          case 'c':
            argvflags.c_flag = 1;
            break;

          case 'V':
            mpgedit_version();
            break;

          case 'l':
            argvflags.l_flag = 1;
            argvflags.l_value = strdup(optarg);
            break;
  
          case 'L':
            argvflags.L_flag = 1;
            break;
  
          case 'D':
            argvflags.D_flag = 1;
            switch (optarg ? optarg[0] : '\0') {
              case 'b':
                intarg = atoi(&optarg[1]) / 6;
                mpgedit_pcmlevel_set_silence_decibels(
                    levelconf, intarg > 0 ? intarg : SILENCE_30DB);
                break;

              case 'c':
                intarg = atoi(&optarg[1]);
                mpgedit_pcmlevel_set_silence_repeat(
                    levelconf, intarg > 0 ? intarg : SILENCE_REPEAT);
                break;

              case 'm':
                intarg = atoi(&optarg[1]);
                mpgedit_pcmlevel_set_minimum_time(
                    levelconf, intarg > 0 ? intarg : MINIMUM_TIME);
                break;

              case 's':
                /*
                 * Decode mp3 file and write output to stdout
                 */
                argvflags.Ds_flag = 1;
                set_stdout_binary();
                break;

              default:
                break;
            }

            if (!argvflags.d_flag) {
                /*
                 * Default decimation level is 9 when decoding mp3 file
                 * to obtain average frame sample levels.
                 */
                argvflags.d_flag = 1;
                argvflags.d_val  = 9;
            }
            break;
  
          case 'd':
            argvflags.d_flag = 1;
            argvflags.d_val  = atoi(optarg);
            break;

          case 'I':
            /* Suppress use of index file during edit */
            argvflags.I_flag = 1;
            break;

          case '?':
            fprintf(stderr, "%s: '-%c' unknown option or missing argument\n", 
                    g_progname, optopt);
            usage(0);
            break;

          default:
            fprintf(stderr, "%s: error parsing command line\n", g_progname);
            usage(0);
            break;
        }
    }
    if (argc == 1) {
        argvflags.c_flag = 1;
    }

    if (argvflags.E_flag) {
        argvflags.e_flag = 0;
    }

    /*
     * It is convenient to treat the first argument following all of the
     * command line options as the input file name.  
     */
    while (optind < argc && argv[optind]) {
        /* If any option is found, that is a fatal error! */
        if (argv[optind][0] == '-') {
            fprintf(stderr, "Unexpected option found after filenames '%s'\n",
                    argv[optind]);
            fprintf(stderr, "Try using '-f' before input file names\n");
            return 1;
        }

        mpgedit_editspec_append(argvflags.edits,
                                mp3edit_alloc_infile(argv[optind]), "");
        optind++;
    }

    if (argvflags.p_flag || argvflags.c_flag) {
        /*
         * Open/close decoder to reveal any decoder open errors.  Also do this
         * when entering curses mode, since it is highly likely the user will
         * want to use the playback feature while in curses mode.
         */
        play_ctx = mpgedit_play_init(NULL, 0);
        if (!play_ctx) {
            fprintf(stderr, "Unable to open playback plugin\n");
            return 1;
        }
        mpgedit_play_close(play_ctx);
    }

    if (argvflags.l_flag) {
        argvflags.vol_left = strtol(argvflags.l_value, &cp, 10);
        if (cp && *cp == ':') {
            cp++;
            argvflags.vol_right = strtol(cp, NULL, 10);
        }
        else {
            argvflags.vol_right = argvflags.vol_left;
        }
        if (argvflags.vol_left < 0 || argvflags.vol_left > 100) {
            argvflags.vol_left = 0;
        }
        if (argvflags.vol_right < 0 || argvflags.vol_right > 100) {
            argvflags.vol_right = 0;
        }
    }

    if (argvflags.c_flag) {
        if (argvflags.e_flag) {
            if (!mpgedit_edit_times_init(argvflags.edits)) {
                fprintf(stderr, "%s: -f filename never specified\n", g_progname);
                return 1;
            }
        }

        status = curs_play(&argvflags,
                           &curs_edits,
                           levelconf);
        if (status) {
            return 1;
        }
        mpgedit_editspec_free(argvflags.edits);
        argvflags.edits  = curs_edits;
        argvflags.e_flag = 1;

        /*
         * Reset -D flag, as this was already delt with in 
         * curses mode.
         */
        argvflags.D_flag = 0;
    }

    /* Silent operation */
    if (argvflags.s_flag) {
        argvflags.v_flag = 0;
        ttyio.printf     = nullprintf;
        ttyio.getch      = NULL;
    }
    else {
        ttyio.printf = myprintf;
        ttyio.getch  = NULL;
    }


    if (argvflags.out_filename &&
        (argvflags.p_flag || (argvflags.X_flag && argvflags.X_val < 2)))
    {
        fprintf(stderr, "%s: -o not allowed with -p or -X\n", g_progname);
        return 1;
    }

    /* -o with no -e not allowed */
    if ((!argvflags.X_flag || (argvflags.X_flag && argvflags.X_val < 2)) &&
        !argvflags.e_flag && argvflags.out_filename)
    {
        fprintf(stderr, "%s: -o must be used with -e\n", g_progname);
        return 1;
    }

    if (argvflags.X_flag && argvflags.X_val == 2) {
        /*
         * Build the Xing header file.  The choice of header template
         * depends on the sample rate and channel mode of the file
         * being patched.  That must first be evaluated.
         */
        next_editspec = mpgedit_editspec_get_edit(
                            argvflags.edits,
                            0,
                            2,
                            0, &next_editspec_len);
        rsts = read_mp3_header_file(mpgedit_editspec_get_file(next_editspec, 1),
                                     &channel_mode, &sample_rate);
        if (rsts == 0) {
            xingheader_file_make3(XING_HEADER_FILE, channel_mode, sample_rate);
        }
    }

    edits_len = mpgedit_editspec_get_length(argvflags.edits);
    if (!edits_len) {
        fprintf(stderr, "%s: no file name specified\n", g_progname);
        return 1;
    }

    if (argvflags.p_flag && argvflags.X_flag) {
        fprintf(stderr, "%s: both -p and -X not allowed\n", g_progname);
        return 1;
    }

    str = malloc(1024);
    if (!str) {
        fprintf(stderr, "failed allocating print string buffer\n");
        return 1;
    }

    /*
     * curs_play() has already done this initialization and
     * validation, so don't duplicate it here.
     */
    if (argvflags.c_flag == 0) {
        /*
         * Process -e and -f options.
         */
        if (!mpgedit_edit_times_init(argvflags.edits)) {
            fprintf(stderr, "%s: -f filename never specified\n", g_progname);
            return 1;
        }

        status = 0;
        if (edits_len > 0) {
            errstr = NULL;
            for (indx=0; !errstr && indx<edits_len; indx++) {
                if (!argvflags.p_flag && !argvflags.e_flag &&
                    !argvflags.v_flag && !no_edits_msg &&
                    !argvflags.D_flag)
                {
                    printf("No edits specified; only reading input data\n\n");
                    no_edits_msg = 1;
                }

                if (!argvflags.I_flag) {
                    ttyio.cb_called = 0; 
                    mpeg_time_clear(&mpegtval);
                    rsts = mp3header_index_create(
                               mpgedit_editspec_get_file(
                                   argvflags.edits, indx),
                               index_callback,
                               &ttyio, &indx_stats, &mpegtval);
                    /*
                     * Set indexed flag here to indicate the file statistics
                     * have been printed here.  Prevents reading file a 
                     * second time.
                     */
                    mpgedit_editspec_set_indexed(
                        argvflags.edits, indx, ttyio.cb_called);


                    if (!argvflags.p_flag && !argvflags.e_flag &&
                        ttyio.cb_called && rsts == 0) 
                    {
                        printf("\ndone.\n");
                        if (!argvflags.v_flag) {
                            mpeg_file_stats2str(&indx_stats, &mpegtval, str);
                            printf("%s", str);
                        }
                    }
                    errstr = mpgedit_edit_validate_times(
                                 argvflags.edits, indx, 1);
                }
            }
        }
        if (errstr) {
            fprintf(stderr, "Error: %s\n", errstr);
            mpgedit_edit_files_free(errstr);
            return 1;
        }
    }

    if (argvflags.out_filename) {
        /*
         * Populate edit_filename and out_fileappend when out_filename
         * is specified
         */
        if (argvflags.out_filename[0] == '+') {
            edit_filename = strdup(&argvflags.out_filename[1]);
            argvflags.out_fileappend = 1;
        }
        else {
            edit_filename = strdup(argvflags.out_filename);
            argvflags.out_fileappend = 0;
        }
        rsts = validate_outfile(argvflags.out_filename,
                                argvflags.out_fileappend, &errstr);
        if (rsts) {
            fprintf(stderr, "%s\n", errstr);
            return rsts;
        }
    }

    /*
     * When only an input file is specified, add -v action.
                if (!argvflags.p_flag && !argvflags.e_flag &&
                    !argvflags.v_flag && !no_edits_msg &&
                    !argvflags.D_flag)
     */
    if (!argvflags.p_flag && !argvflags.e_flag && !argvflags.D_flag) {
        if (!argvflags.v_flag) {
            if (!no_edits_msg) {
                printf("No edits specified; only reading input data\n\n");
            }
            if (!argvflags.X_flag) {
                /* Just return file statistics already computed and saved in index */
                edit_flags |= MPGEDIT_FLAGS_GET_STATS;
            }
        }
        edit_flags |= MPGEDIT_FLAGS_NO_EDITS;

    }

    edit_indx    = 0;
    edit_ctx     = NULL;
    outfile_indx = 1;

    /* When very verbose, print output from every frame */
    if (argvflags.v_flag > 1) {
        edit_flags |= MPGEDIT_FLAGS_EACH_FRAME;
    }

    /*
     * When very, very, very, very verbose, print md5 checksum of each frame.
     * -vvvv is getting rediculous.  However, these options are part of the
     * mpgedit command line interface, so the existing semantic cannot be
     * changed.  Of course, there is also -vvvvv, yikes. 
     */
    if (argvflags.v_flag > 3) {
        edit_flags |= MPGEDIT_FLAGS_MD5SUM_FRAME;
    }

    if (edit_filename) {
        edit_outfilename = strdup(edit_filename);
    }
    else {
        if (mpgedit_editspec_get_file(argvflags.edits, 0)) {
            edit_filename =
                strdup(mpgedit_editspec_get_file(argvflags.edits, 0));
            cp = strrchr(edit_filename, '.');
            if (cp) {
                *cp = '\0';
            }
        }
    }

    if (argvflags.X_flag) {
        if (argvflags.X_val == 0) {
            edit_flags |= MPGEDIT_FLAGS_XING_OMIT;
        }
        else if (argvflags.X_val == 1) {
            edit_flags |= MPGEDIT_FLAGS_XING_FIXUP;
        }
        else if (argvflags.X_val == 2) {
            if (edits_len != 2) {
                fprintf(stderr,
                        "%s: Must specify exactly one input "
                        "file with -X2 option\n", g_progname);
                return 1;
            }
            /*
             * For -X2 option, create unique output file name based on input
             * file name to save output with restored Xing header into.
             */
            if (!argvflags.out_filename) {
                edit_outfilename = build_edit_filename(
                                       mpgedit_editspec_get_file(
                                           argvflags.edits, 1), "_newxing");
                argvflags.out_filename = strdup(edit_outfilename);
            }
        }
    }

    if (argvflags.I_flag) {
        edit_flags |= MPGEDIT_FLAGS_NO_INDEX;
        memset(&seekcb_ctx, 0, sizeof(seekcb_ctx));
    }

    memset(&edit_stats, 0, sizeof(edit_stats));
    do {
#if 1
        next_editspec = mpgedit_editspec_get_edit(
                            argvflags.edits,
                            edit_indx,
                            edits_len,
                            0, &next_editspec_len);
#else
        next_editspec = mpgedit_editspec_get_edit(
                            &argvflags.edits[edit_indx],
                            argvflags.edits_cnt - edit_indx,
                            0, &next_editspec_len);
#endif
        if (!next_editspec) {
/* adam/TBD: allocation error */
        }

        if (!argvflags.out_filename) {
/*
 * adam/TBD: This is where you will add special logic for new
 * option that disables the automatic sequential numbering of the
 * output files.
 */
#if 1
            edit_outfilename = build_edit_filename(edit_filename, NULL);
#else
            edit_outfilename = mpgedit_edit_make_output_filename(
                                   ".", edit_filename,
                                   "mp3", &outfile_indx);
#endif
        }

        
        if (argvflags.Ds_flag) {
            decode_files(next_editspec, &argvflags, &ttyio);
        }
        else if (argvflags.D_flag) {
            decode_files(next_editspec, &argvflags, &ttyio);
            find_segment_boundaries(next_editspec, &argvflags, levelconf);
            tprev = 0; /* adam/TBD: what is tprev used for ?? */
        }
        else if (argvflags.p_flag) {
            play_files(next_editspec, &argvflags);
            tprev = 0; /* adam/TBD: what is tprev used for ?? */
        }
        /*
         * Edits or verbose file statistics have been specified, or the file
         * statistics have not already been printed by the indexing operation
         * when no file operation has been specified.
         */
        else if (argvflags.e_flag || argvflags.v_flag || 
                 (next_editspec_len == 1 && 
                  mpgedit_editspec_get_indexed(next_editspec, 0) == 0))
        {
            if ((edit_indx > 0 && argvflags.out_filename) ||
                argvflags.out_fileappend)
            {
                edit_flags |= MPGEDIT_FLAGS_APPEND;
            }

            if (!argvflags.s_flag && argvflags.e_flag &&
                 !(argvflags.X_val == 2 &&
                   strcmp(mpgedit_editspec_get_file(next_editspec, 0),
                          XING_HEADER_FILE) == 0))
            {
                printf("Writing edits to file '%s'\n", edit_outfilename);
            }
            edit_ctx = mpgedit_edit_files_init5(next_editspec,
                                               edit_outfilename,
                                               edit_flags,
                                               &edit_stats,
                                               &rsts);
            if (argvflags.I_flag && !argvflags.s_flag) {
                seekcb_ctx.ctx = edit_ctx;
                mpgedit_edit_set_seek_callback(edit_ctx,
                                               seek_callback, &seekcb_ctx);
            }
            editing = 1;
            while (rsts == 0 && editing) {
                editing = mpgedit_edit_files(edit_ctx, &rsts);
                if (rsts == 0) {
                    mp3header_print_verbose(
                        edit_ctx,
                        argvflags.s_flag ? -1 : argvflags.v_flag,
                                                &once,
                        &tprev);
                }
            }
            if (rsts == 0) {
                if ((!argvflags.s_flag && 
                      !(argvflags.X_val == 2 && 
                        strcmp(mpgedit_editspec_get_file(
                               next_editspec, 0),
                               XING_HEADER_FILE) == 0)) || 
                    (argvflags.s_flag && !argvflags.e_flag))
                {
                    mpgedit_edit_stats2str(
                        edit_ctx,
                        mpgedit_edit_total_length_time(edit_ctx),
                        str);
                    printf("%s", str);
                }
            }
            else {
                printf("mpgedit failed: status = %d\n", rsts);
            }
            mpgedit_free(edit_ctx);
        }
        if (next_editspec) {
            mpgedit_edit_files_free(next_editspec);
        }
        if (!argvflags.out_filename && edit_outfilename) {
            free(edit_outfilename);
            edit_outfilename = NULL;
        }
        edit_indx += next_editspec_len;
    } while (edit_indx < edits_len);
    if (edit_outfilename) {
        free(edit_outfilename);
        edit_outfilename = NULL;
    }
    if (argvflags.out_filename) {
        free(argvflags.out_filename);
    }
    if (edit_filename) {
        free(edit_filename);
    }
    
    mpgedit_editspec_free(argvflags.edits);

    if (editfp) {
        fclose(editfp);
        editfp = NULL;
    }
    free(str);

    return 0;
}

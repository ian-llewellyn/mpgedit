/*
 * mp3get: streaming mp3 download utility makefile
 *
 * Copyright (C) 2004 Adam Bernstein
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
#include <string.h>
#include <stdlib.h>
#include "mpg123.h"
#include <portability.h>


char *prgName = "mp3get";
char *prgVersion = "0.01";

#ifdef WIN32
#include <fcntl.h>
#include <time.h>

#else

#include <unistd.h>

#endif


int read_stream(int fd, int max, long *duration, FILE *outfp)
{
    char buf[1024];
    size_t cnt;
    int total = 0;
    int done = 0;
    time_t tstart;
    time_t tend;
    time_t tnow;

    if (duration) {
        tstart = time(NULL);
        tend = tstart + (time_t) (*duration);
    }
    do {
        cnt = recv(fd,  buf, sizeof(buf), 0);
        if (cnt > 0) {
            fwrite(buf, 1, cnt, outfp);
            fflush(outfp);
            total += cnt;
        }

        if (duration) {
            tnow = time(NULL);
            done = tnow > tend;
        }
        else if (max && (total > max)) {
            done = 1;
        }
    } while (cnt > 0 && !done);
    return duration ? tnow - tstart : total;
}


void usage(int optch)
{
    if (optch) {
        fprintf(stderr, "mp3get: unknown option '%c'\n", optch);
    }
    fprintf(stderr, "usage: mp3get [-o outfile] [-s save_bytes] [-S save_secs] URL\n");
    exit(1);
}


int main(int argc, char *argv[])
{
    int  fd;
    int  retry = 0;
    FILE *outfp = stdout;
    char *file  = NULL;
    char *filebase = NULL;
    int  optch;
    int  file_max = 0;
    long duration;
    long *pduration = NULL;
    long val;

    /* adam/TBD: make header file */
    extern int http_open (char *url);


    if (argc == 1) {
        usage('\0');
        return 1;
    }

    opterr = 0;
    while ((optch = getopt(argc, argv, "S:s:o:")) != -1) {
        switch (optch) {
          case 's':
            file_max = atoi(optarg);
            break;

          case 'S':
            /*
             * Collect S seconds worth of data.  This is often results in
             * a file with a longer play time than S, probably because the
             * streaming server dumps some buffered data at connect time.
             */
            duration = strtol(optarg, NULL, 0);
            pduration = &duration;
            break;

          case 'o':
            filebase = (char *) strdup(optarg);
            file = (char *) malloc(strlen(optarg) + 6);
            if (!file || !filebase) {
                perror("Failed allocating output filename");
                return 1;
            }
            strcpy(file, optarg);
            break;

          default:
          case '?':
            usage(optopt);
            break;
 
        }
    }

    if (!argv[optind]) {
        fprintf(stderr, "mp3get: URL not specified\n");
        usage(0);
    }

    retry = 0;
    do {
        fd = http_open(argv[optind]);
        if (fd == -1) {
            fprintf(stderr, "mp3get: failed opening URL '%s'\n",
                    argv[optind]);
            return 1;
        }
    
        if (file) {
            outfp = fopen(file, "wb");
            if (!outfp) {
                perror("Failed opening output file");
                return 1;
            }
        }
        else {
            set_stdout_binary();
        }
    
        val = read_stream(fd, file_max, pduration, outfp);
        if ((pduration && (val < *pduration)) || (val < file_max)) {
            fclose(outfp), outfp = NULL;
            close(fd);
            retry++;
            sprintf(file, "%s.%d", filebase, retry);
            /*
             * Disconnected prematurely. Reconnect, and try to read the
             * residual.
             */
            if (pduration) {
                *pduration -= val;
            }
            else {
                file_max -= val;
            }
        }
    } while ((pduration && (val < *pduration)) || (val < file_max));
    if (outfp) {
        fclose(outfp);
    }
    free(file);
    free(filebase);
    return 0;
}

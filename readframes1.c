/*
 * MPEG frame read/decode test program
 *
 * Copyright (C) 2005 Adam Bernstein
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


#include "header.h"
#include "portability.h"
#include "mpegfio.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include <mad.h>

#define OUTPUT_BUFFER_SIZE 8192 /* Must be an integer multiple of 4. */
#define MAX_MP3_SIZE 2048       /* Slightly larger than max MP3 frame of 2016 bytes */
#define OPTARGS "+dvo:f:"       /* Leading plus forces POSIX processing of argument list */


typedef struct _mpgedit_madbuf_t {
    unsigned char     stream_buf[MAX_MP3_SIZE*2];

    unsigned char     OutputBuffer[OUTPUT_BUFFER_SIZE];
    unsigned char     *OutputPtr;
    unsigned char     *OutputBufferEnd;
    int               sbi;                               /* stream_buf index */
    int               frame_count;
    int               verbose;
    int               silent;
    char              *string;
    FILE              *infp;
    FILE              *outfp;
    mpeg_header_data  stats_header;
    mpeg_file_iobuf   mpegiobuf;

    struct mad_stream Stream;
    struct mad_frame  Frame;
    struct mad_synth  Synth;
    mad_timer_t       Timer;
} mpgedit_madbuf_t;


void printf_maderrstr(int error)
{
    printf("\nDecode error: sts=%04x ", error);
    switch (error) {
      case MAD_ERROR_BUFLEN:
        printf("<input buffer too small>");
        break;
      case MAD_ERROR_BADHUFFDATA:
        printf("<Huffman data overrun>");
        break;
      default:
        break;
     }
     printf("\n");
}


/* A fixed point number is formed of the following bit pattern:
 *
 * SWWWFFFFFFFFFFFFFFFFFFFFFFFFFFFF
 * MSB                          LSB
 * S ==> Sign (0 is positive, 1 is negative)
 * W ==> Whole part bits
 * F ==> Fractional part bits
 *
 * This pattern contains MAD_F_FRACBITS fractional bits, one
 * should alway use this macro when working on the bits of a fixed
 * point number. It is not guaranteed to be constant over the
 * different platforms supported by libmad.
 *
 * The signed short value is formed, after clipping, by the least
 * significant whole part bit, followed by the 15 most significant
 * fractional part bits. Warning: this is a quick and dirty way to
 * compute the 16-bit number, madplay includes much better
 * algorithms.
 */
#define MadFixedToSshort(Fixed) \
    ((Fixed) >= MAD_F_ONE)  ? SHRT_MAX  : \
    ((Fixed) <= -MAD_F_ONE) ? -SHRT_MAX : ((signed short)((Fixed)>>(MAD_F_FRACBITS-15)))


int init_mp3_data_buffers(char *outfile, mpgedit_madbuf_t *ctx)
{
    memset(ctx, 0, sizeof(*ctx));

    ctx->string          = malloc(1024);
    if (!ctx->string) {
        return 1;
    }
    ctx->OutputPtr       = ctx->OutputBuffer;
    ctx->OutputBufferEnd = ctx->OutputPtr + sizeof(ctx->OutputBuffer);
    if (outfile[0] == '-' && outfile[1] == '\0') {
        ctx->outfp = stdout;
    }
    else {
        ctx->outfp = fopen(outfile, "wb");
    }
    return ctx->outfp ? 0 : 1;
}


void free_mp3_data_buffers(mpgedit_madbuf_t *ctx)
{
    free(ctx->string);
    if (ctx->outfp != stdout) {
        fclose(ctx->outfp);
    }
}


int fill_mp3_data_buffers(mpgedit_madbuf_t *ctx)
{
    int              found;
    int              eof = 0;
    mpeg_header_data header;

    memset(&header, 0, sizeof(header));
    do {
        found = mpeg_file_next_frame_read(ctx->infp, &ctx->mpegiobuf,
                                          &ctx->stats_header, &eof);
        if (found && !eof) {
            ctx->frame_count++;
            if (mpgedit_header_decode(mpeg_file_iobuf_getptr(&ctx->mpegiobuf), 
                                      &header, MPGEDIT_ALLOW_MPEG1L1)) 
            {
                if (ctx->verbose == 2) {
                    mpeg_header_values2str(&header, ctx->string);
                }
                if (ctx->sbi == 0) {
                    memcpy(ctx->stream_buf, mpeg_file_iobuf_getptr(&ctx->mpegiobuf), 
                           header.frame_size);
                    ctx->sbi = header.frame_size;
                }
                else if (ctx->sbi < MAX_MP3_SIZE) {
                    memcpy(&ctx->stream_buf[ctx->sbi], mpeg_file_iobuf_getptr(&ctx->mpegiobuf), 
                           header.frame_size);
                    ctx->sbi += header.frame_size;
                }
                mpeg_file_iobuf_setstart(&ctx->mpegiobuf,
                                         mpeg_file_iobuf_getstart(&ctx->mpegiobuf) +
                                                   header.frame_size);
                header.position += header.frame_size;

                if (ctx->verbose == 2) {
                    printf("Frame: %-4d\n", ctx->frame_count);
                    printf("%s", ctx->string);
                }

                if (ctx->sbi > MAX_MP3_SIZE/2) {
                    return 0;
                }
            }
        }
    } while (!eof);
    return eof;
}


int output_synth_data(mpgedit_madbuf_t *ctx)
{
    int i;
    int rsts;

    for(i=0;i<ctx->Synth.pcm.length;i++) {
        signed short    Sample;

        /* Left channel */
        Sample=MadFixedToSshort(ctx->Synth.pcm.samples[0][i]);
        *(ctx->OutputPtr++) = Sample & 0xff;
        *(ctx->OutputPtr++) = Sample >> 8;

        /* Right channel. If the decoded stream is monophonic then
         * the right output channel is the same as the left one.
         */
        if(MAD_NCHANNELS(&ctx->Frame.header)==2) {
            Sample=MadFixedToSshort(ctx->Synth.pcm.samples[1][i]);
            *(ctx->OutputPtr++) = Sample & 0xff;
            *(ctx->OutputPtr++) = Sample >> 8;
        }

        /* Flush the output buffer if it is full. */
        if (ctx->OutputPtr >= ctx->OutputBufferEnd) {
            rsts = fwrite(ctx->OutputBuffer, 1, OUTPUT_BUFFER_SIZE, ctx->outfp);
            if (rsts != OUTPUT_BUFFER_SIZE) {
                fprintf(stderr,"%s: PCM write error (%s).\n",
                        "main" ,strerror(errno));
                return 1;
            }
            ctx->OutputPtr = ctx->OutputBuffer;
        }
    }
    return 0;
}


int decode_mp3_stream(mpgedit_madbuf_t *ctx)
{
    int rsts;
    int unrecoverable = 1;
    int adjust_stream = 0;

    mad_stream_buffer(&ctx->Stream, ctx->stream_buf, ctx->sbi);
    rsts = mad_frame_decode(&ctx->Frame, &ctx->Stream);
    if (rsts) {
        if (!ctx->silent) {
            printf_maderrstr(ctx->Stream.error);
        }
        if (MAD_RECOVERABLE(ctx->Stream.error)) {
            if (ctx->Stream.error == MAD_ERROR_BADDATAPTR ||
                ctx->Stream.error == MAD_ERROR_LOSTSYNC)
            {
                memmove(ctx->stream_buf, ctx->stream_buf + 1, ctx->sbi - 1);
                ctx->sbi--;
                if (!ctx->silent) {
                    printf("ctx->sbi=%d\n", ctx->sbi);
                }
            }
            else {
                adjust_stream = 1;
            }
            unrecoverable = 0;
        }
        else if (ctx->Stream.error == MAD_ERROR_BUFLEN) {
            unrecoverable = 10;
            adjust_stream = 1;
            if (!ctx->silent) {
                printf("Decode ok...\n");
            }
        }
        else {
            if (!ctx->silent) {
                printf("Decode failed\n");
            }
        }
    }
    else {
        unrecoverable = 0;
        adjust_stream = 1;
    }

    if (adjust_stream) {
        if (ctx->Stream.next_frame) {
            ctx->sbi = ctx->Stream.bufend - ctx->Stream.next_frame;
            memcpy(ctx->stream_buf, ctx->Stream.next_frame, ctx->sbi);
        }
        else {
            ctx->sbi = 0;
        }
    }
    return unrecoverable;
}


void usage(char *msg)
{

    if (msg) {
        printf("%s", msg);
    }
    printf("usage: readheader1 [-d] [-f infile.mp3] [-o decode.wav] [-v[-v]] mp3_file\n");
    exit(1);
}


int main(int argc, char *argv[])
{
    int              eof = 0;
    mpgedit_madbuf_t md;
    int              rsts;
    int              decode = 0;
    int              verbose = 0;
    int              outfile = 0;
    int              ch;
    char             *ofile = strdup("/tmp/decode.wav");
    char             *ifile = NULL;
    int              i;

    memset(&md, 0, sizeof(md));
    mad_stream_init(&md.Stream);
    mad_frame_init(&md.Frame);
    mad_synth_init(&md.Synth);
    mad_timer_reset(&md.Timer);

    while ((ch = getopt(argc, argv, OPTARGS)) != -1) {
        switch (ch) {
          case 'd': /* Decode file */
            decode = 1;
            break;

          case 'f': /* Input file */
            ifile = strdup(optarg);
            break;

          case 'o': /* Output file */
            ofile = strdup(optarg);
            outfile = 1;
            decode = 1;
            break;

          case 'v': /* Verbose */
            verbose++;
            break;

          case '?': 
            usage(NULL);
            break;

          default:
            break;
        }
    }

    if (!outfile && decode) {
        printf("Decoding output to '%s'\n", ofile);
    }

    if (!ifile) {
        if (optind >= argc) {
            usage("Error: Input file name not specified\n");
        }
        else {
            ifile = strdup(argv[optind]);
        }
    }

    /* Do something useful if an input file is specified, but decode was not */
    if (ifile && !decode) {
        verbose = 2;
    }


    rsts = init_mp3_data_buffers(ofile, &md);
    free(ofile);
    if (rsts) {
        printf("failed opening output file\n");
        return 1;
    }

    /* Must not be verbose when writing output to stdout */
    if (md.outfp == stdout) {
        set_stdout_binary();
        verbose = 0;
        md.silent = 1;
    }
    md.verbose = verbose;

    md.infp = mpeg_file_open(ifile, "rb");
    free(ifile);
    if (!md.infp) {
        perror("fopen failed");
        return 1;
    }

    i = 0;
    do {
        if (md.sbi < MAX_MP3_SIZE/2 || rsts == 10) {
            eof = fill_mp3_data_buffers(&md);
        }
        if (decode && !eof) {
            rsts = decode_mp3_stream(&md);
            if (rsts == 0 || rsts == 10) {
                mad_synth_frame(&md.Synth, &md.Frame);
                output_synth_data(&md);
            }
        }
        if (!decode) {
            md.sbi = 0;
        }
        if (rsts == 1) {
            eof = rsts;
            if (!md.silent) {
                printf("Error decoding stream!\n");
            }
        }
        if (md.verbose == 1) {
            printf("%5d", md.sbi);
            if ((i++ % 16) == 0) {
                printf("\nFrame %6d: ", md.frame_count);
            }
        }
    } while (!eof);
    if (md.verbose == 1) {
        printf("\n");
    }
    free_mp3_data_buffers(&md);
    mpeg_file_close(md.infp);
    return 0;
}

/*
 * mpgedit file I/O and buffering routines
 *
 * Copyright (C) 2001 Adam Bernstein
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
static char SccsId[] = "$Id: mpegfio.c,v 1.11 2004/11/27 21:13:29 number6 Exp $";
#endif

#include <stdio.h>
#include <string.h>
#include "mpegfio.h"
#include "header.h"

#ifndef mpeg_file_iobuf_clear
void mpeg_file_iobuf_clear(mpeg_file_iobuf *buf)
{
    buf->start = buf->len = 0;
}
#endif


#ifndef mpeg_file_iobuf_getptr
unsigned char *mpeg_file_iobuf_getptr(mpeg_file_iobuf *buf)
{
    return buf->buf + buf->start;
}
#endif


#ifndef mpeg_file_iobuf_setstart
void mpeg_file_iobuf_setstart(mpeg_file_iobuf *buf, int start)
{
    buf->start = start;
}
#endif

#ifndef mpeg_file_iobuf_getstart
int mpeg_file_iobuf_getstart(mpeg_file_iobuf *buf)
{
    return buf->start;
}
#endif


#ifndef mpeg_file_iobuf_getlen
int mpeg_file_iobuf_getlen(mpeg_file_iobuf *buf)
{
    return buf->len - buf->start;
}
#endif


#ifndef mpeg_file_iobuf_buflen
int mpeg_file_iobuf_buflen(mpeg_file_iobuf *buf)
{
    return sizeof(buf->buf);
}
#endif


int mpeg_file_iobuf_read(FILE *fp, mpeg_file_iobuf *buf)
{
    int sts = 1;

    if (feof(fp)) {
        sts = 0;
        goto clean_exit;
    }
    if (buf->start == 0 && buf->len == 0) {

        /* Fill empty buffer */

        buf->len = fread(buf->buf, 1, sizeof(buf->buf), fp);
        if (buf->len == 0 && feof(fp)) {
            sts = 0;
            goto clean_exit;
        }
    }
    else if (buf->start < buf->len) {
        /*
         * Move remaining data up to start of buffer, then fill in
         * remaining space.
         */
         memmove(buf->buf, buf->buf+buf->start, buf->len-buf->start);
         buf->len  = buf->len - buf->start;
         buf->len += fread(buf->buf + buf->len, 1, buf->start, fp);
         buf->start = 0;
    }
    else {
        sts = 0;
    }


clean_exit:
    return sts;
}


FILE *mpeg_file_open(char *file, char *mode)
{
    if (!file) {
        return NULL;
    }
    return fopen(file, mode);
}


void mpeg_file_close(FILE *fp)
{
    if (fp) {
        fclose(fp);
    }
}


int read_next_mpeg_frame(FILE *fp,
                         mpeg_file_iobuf *mpegiobuf,
                         mpeg_header_data *ret_header,
                         int *eof)
{
    int found   = 0;
    int sts;
    int not_eof = 1;
    int wsize;
    char *cp;
    mpeg_file_iobuf  saved_mpegiobuf;
    int              saved_data = 0;
    mpeg_header_data saved_header;
    mpeg_header_data header;
  
    memset(&header, 0, sizeof(header));
    do {
        if (mpeg_file_iobuf_getlen(mpegiobuf) <= 4 || found == -1) {
            /*
             * Not a full MPEG frame header present in the buffer, so read
             * the next buffer's worth of data from the file.  Advance
             * the input buffer by offset, so the residual data is preserved.
             */
            if (header.position > 0) {
                mpeg_file_iobuf_setstart(mpegiobuf, header.position);
            }
            not_eof = mpeg_file_iobuf_read(fp, mpegiobuf);
        }
        header.position = 0;

        /*
         * Search for the next MPEG frame header in the buffer of data.
         */
        if (not_eof) {
            sts = find_mpeg_header_buf(mpeg_file_iobuf_getptr(mpegiobuf),
                                       mpeg_file_iobuf_getlen(mpegiobuf),
                                       &header, MPGEDIT_ALLOW_MPEG1L1);
    
            if (sts == 0) {
                /*
                 * Have not yet found the start of the MPEG frame.   Move the
                 * data buffer cursor forward over data already processed.
                 */
                mpeg_file_iobuf_setstart(
                    mpegiobuf,
                    mpeg_file_iobuf_getstart(mpegiobuf) +
                    mpeg_file_iobuf_getlen(mpegiobuf) - 4);
            }
            else {
                found        = sts;
                saved_header = header;

                /*
                 * When find_mpeg_header_buf() returns -1, this is
                 * an indication not all of the frame data is present,
                 * or the previous complete frame is not followed by
                 * another valid frame.  This can happen at EOF,
                 * or when the last valid frame is followed by other
                 * non-MPEG frame data, like ID3 tag information.
                 * However, when the mpegiobuf is full, there is a 
                 * problem.  We can't tell if we are just the last
                 * valid frame of data in the file, followed by
                 * other non-MPEG data, or we are still searching for
                 * the first valid  MPEG frame, and this is just
                 * random data that appears to be a valid frame.
                 * Save this buffer, and treat it is the last valid 
                 * frame of data should EOF be encountered.
                 */
                if (sts == -1 && 
                    mpeg_file_iobuf_getlen(mpegiobuf) ==
                         mpeg_file_iobuf_buflen(mpegiobuf))
                {
                     saved_mpegiobuf = *mpegiobuf;
                     saved_data      = 1;

                     /*
                      * Walk over frame header present in buffer. This
                      * makes more room in the buffer for the next read.
                      */
                     mpeg_file_iobuf_setstart(mpegiobuf, header.frame_size);
                }
            }
        }
    } while (found != 1 && not_eof);

    *eof = not_eof == 0;

    if (*eof && found == -1) {
        /*
         * Because there is no frame following the last frame in a file,
         * found will be -1.
         */
        found = 1;
    }
    if (found) {
        header = saved_header;
        if (saved_data) {
            *mpegiobuf  = saved_mpegiobuf;
        }
        if (header.position > 0) {
            mpeg_file_iobuf_setstart(mpegiobuf,
                                     mpeg_file_iobuf_getstart(mpegiobuf)
                                         + header.position);
        }
        wsize = mpeg_file_iobuf_getlen(mpegiobuf);
        if (header.frame_size > wsize) {
            header.frame_size = wsize;
        }

        /*
         * Test for an ID3 tag at the end of file.  When found, remove
         * the last 128 bytes of data from the valid data length.
         */
        if (*eof && wsize >= 128) {
            cp = (char *) mpeg_file_iobuf_getptr(mpegiobuf) + (wsize - 128);
            if (!strncmp(cp, "TAG", 3)) {
                wsize -= 128;
                if (header.frame_size > wsize) {
                    header.frame_size = wsize;
                }
            }
        }
        *ret_header = header;
    }
    return found;
}


/*
 * Public API for read_next_mpeg_frame
 */
int mpeg_file_next_frame_read(FILE *fp,
                              mpeg_file_iobuf *mpegiobuf,
                              mpeg_header_data *ret_header,
                              int *eof)
{
    return read_next_mpeg_frame(fp,
                                mpegiobuf,
                                ret_header,
                                eof);
}



int mpegfio_has_xing_header(FILE *fp,
                            mpeg_header_data *mpeg_header,
                            XHEADDATA *xingh, int append)
{
    int status = 0;
    int cnt = 0;
    long curpos;
    unsigned char    mpeg_header_data[4];

    if (!fp) {
        return status;
    }
    
    /*
     * Remember where we are in the file; may need to return here.
     */
    curpos = ftell(fp);

    /*
     * Test to see if this is a VBR file with XING header.
     * When appending to the end of a file, read the header from the
     * target file, and update  the current edits stats in that header.
     */
    memset(xingh, 0, sizeof(*xingh));
    if (append) {
        fseek(fp, 0, SEEK_SET);
    }

    cnt = fread(mpeg_header_data, sizeof(mpeg_header_data), 1, fp);
    if (cnt != 1) {
        goto clean_exit;
    }
    cnt = decode_mpeg_header(mpeg_header_data, mpeg_header, MPGEDIT_ALLOW_MPEG1L1);
    if (!cnt) {
        goto clean_exit;
    }
    memcpy(xingh->xingbuf, mpeg_header_data, sizeof(mpeg_header_data));
    if (mpeg_header->frame_size-4 <= 0) {
        goto clean_exit;
    }
    cnt = fread(&xingh->xingbuf[4], mpeg_header->frame_size - 4, 1, fp);
    if (!cnt) {
        goto clean_exit;
    }
    if (append) {
        fseek(fp, 0, SEEK_END);
    }

    status = (xingheader_init(xingh->xingbuf, 
                              mpeg_header->frame_size, xingh) > 0);
    /*
     * Return to same position in data stream when no header is present 
     */
clean_exit:
    if (!status) {
        fseek(fp, curpos, SEEK_SET);
    }

    return status ? mpeg_header->frame_size : 0;
}

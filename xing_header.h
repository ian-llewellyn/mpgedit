/*
 * XING header manipulation routines header
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

/*
 * SccsId[] = "$Id: xing_header.h,v 1.9.6.1 2009/03/14 07:56:26 number6 Exp $"
 */

#ifndef _XING_HEADER_H_
#define _XING_HEADER_H_ 1


#define FRAMES_FLAG     0x0001
#define BYTES_FLAG      0x0002
#define TOC_FLAG        0x0004
#define VBR_SCALE_FLAG  0x0008

#include "portability.h"
#include "mpegstat.h"

#define XING_HEADER_FILE "xingheader.mp3"

#define FRAMES_AND_BYTES (FRAMES_FLAG | BYTES_FLAG)

/*
11111111 111BBCCD EEEEFFGH IIJJKLMM 
BB = MPEG Audio version ID
     00 - MPEG Version 2.5
     01 - reserved
     10 - MPEG Version 2 (ISO/IEC 13818-3)
     11 - MPEG Version 1 (ISO/IEC 11172-3) 

CC = Layer description
     00 - reserved
     01 - Layer III
     10 - Layer II
     11 - Layer I

D =  Protection bit
     0 - Protected by CRC (16bit crc follows header)
     1 - Not protected
*/

typedef struct {
     int h_id;            /* from MPEG header, 0=MPEG2, 1=MPEG1 (BB) */
     int h_layer;         /* from MPEG header, 1=layer1, 2=layer2, 3=layer3 (CC) */
     int h_protect;       /* from MPEG header, 1=protected, 0=not protected */
     int h_mode;          /* from MPEG header, 3=mono, otherwise stereo */
     int samprate;        /* determined from MPEG header */
     int flags;           /* from Xing header data */
     int frames;          /* total bit stream frames from Xing header data */
     int bytes;           /* total bit stream bytes from Xing header data */
     int vbr_scale;       /* encoded vbr scale from Xing header data */
     unsigned char *toc;  /* pointer to unsigned char toc_buffer[100] */
                          /* may be NULL if toc not desired */

     /*
      * Cannot use XING_HEADER_SIZE for this buffer; may be bigger
      * than "128K" frame size.
      */
     unsigned char xingbuf[2048];
     int           xingbuflen;
     int           xing_info_header; /* Set to 1 if CBR "Info" header is found */
     void          *play_ctx;
}  XHEADDATA;

void InsertI4(int x, unsigned char *buf);
int  ExtractI4(unsigned char *buf);
int  ExtractB12(unsigned char *buf);
void InsertB12(int x, unsigned char *buf);
int  ExtractB20(unsigned char *buf);
void InsertB20(int x, unsigned char *buf);
int  ExtractI2(unsigned char *buf);
void InsertI2(int x, unsigned char *buf);
void ExtractB2210(unsigned char *buf, int *rx, int *ry);
void InsertB2210(unsigned char *buf, int x, int y);

_DSOEXPORT int _CDECL xingheader_file_make(char *file);
_DSOEXPORT int _CDECL xingheader_file_make3(char *file,
                                            int channel_mode,
                                            int sample_rate);


void xingheader_edit(XHEADDATA *xingh, 
                     int frames, int bytes, int version, int mode);
int  xingheader_init(unsigned char *buf, int len, XHEADDATA *X);
int  xingheader_parse(XHEADDATA *xingh);
int  xingheader_init_mpgedit_ctx(XHEADDATA *xingh, void *ctx);
_DSOEXPORT void _CDECL xingheader2str(XHEADDATA *xingh, char *cp);

void edit_xing_header(int frames, int bytes,
                      unsigned char *buf, int len,
                      int version, int mode, int is_vbr);


_DSOEXPORT void _CDECL xingheader_stats_edit(XHEADDATA *xingh,
                           mpeg_file_stats *stats,
                           int frames,
                           int bytes,
                           int id,
                           int mode);

#endif

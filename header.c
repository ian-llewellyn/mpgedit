/*
 * MPEG audio frame header parse and decode functions
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

#include <stdio.h>
#include <string.h>
#include "header.h"

#define MPEG_1                  0
#define MPEG_2                  1
#define MPEG_25                 2
#define MPEG_VERSION_RESERVED   3

#define MPEG_LAYER_1            1
#define MPEG_LAYER_2            2
#define MPEG_LAYER_3            3

static int sample_rates[4][3] = {
  /* MPEG 1 values */
  {44100, 48000, 32000},

  /* MPEG 2 values */
  {22050, 24000, 16000},

  /* MPEG 2.5 values */
  {11025, 12000, 8000},

  /* MPEG RESERVED */
  {0, 0,  0}
};

static int bit_rates[6][16] = {
  /* MPEG 1 layer 1 */
  {1, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0 },

  /* MPEG 1 layer 2 */
  {1, 32, 48, 56, 64,  80,  96,  112, 128, 160, 192, 224, 256, 320, 384, 0 },

  /* MPEG 1 layer 3 */
  {1, 32, 40, 48, 56,  64,  80,  96,  112, 128, 160, 192, 224, 256, 320, 0 },

  /* MPEG 2/2.5 layer 1 */
  {1, 32, 48, 56, 64,  80,  96,  112, 128, 144, 160, 176, 192, 224, 256, 0 },

  /* MPEG 2/2.5 layer 2 and 3 */
  {1, 8,  16, 24, 32,  40,  48,  56,  64,  80,  96,  112, 128, 144, 160, 0 },

  /* MPEG RESERVED */
  {0, 0,  0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 }
};


int get_bitrate_from_index(int version, int layer, int index)
{
    int bitrate = 0;

    switch(version) {
      case MPEG_1:
        switch(layer) {
          case MPEG_LAYER_1:
            bitrate = bit_rates[0][index];
            break;
          case MPEG_LAYER_2:
            bitrate = bit_rates[1][index];
            break;
          case MPEG_LAYER_3:
            bitrate = bit_rates[2][index];
            break;
          default:
            break;
        }
        break;
       
      case MPEG_2:
      case MPEG_25:
        switch(layer) {
          case MPEG_LAYER_1:
            bitrate = bit_rates[3][index];
            break;

          case MPEG_LAYER_2:
          case MPEG_LAYER_3:
            bitrate = bit_rates[4][index];
            break;
        }
        break;

      case MPEG_VERSION_RESERVED:
      default:
        bitrate = bit_rates[5][index];
        break;
      
    }
    return bitrate;
}


int mpeg_compute_frame_size(int version,
                            int layer,
                            int bit_rate, 
                            int sample_rate,
                            int pad)
{
    int fsize = 0;

    switch (version) {
      case MPEG_1:
        switch(layer) {
          case MPEG_LAYER_1:
            fsize = (12 * bit_rate * 1000 / sample_rate + pad) * 4;
            break;
    
          case MPEG_LAYER_2:
          case MPEG_LAYER_3:
            fsize = (144 * bit_rate * 1000 / sample_rate) + pad;
            break;

          default:
            break;
        }
        break;

      case MPEG_2:
      case MPEG_25:
        switch(layer) {
          case MPEG_LAYER_1:
            fsize = (12 * bit_rate * 1000 / sample_rate + pad) * 4;
            break;

          case MPEG_LAYER_2:
            fsize = 144 * bit_rate * 1000 / sample_rate + pad;
            break;

          case MPEG_LAYER_3:
            fsize = 72 * bit_rate * 1000 / sample_rate + pad;
            break;

          default:
            break;
        }
        break;

      default:
        break;
    }
    return fsize;
}



/* 
 * This function inspects 4 bytes of data, and inteprets the data 
 * as an MPEG header.  The results of this interpretation are stored
 * in the mpeg_header_data structure.
 * 
 * 1 is returned if the data is an MPEG header
 * 0 is returned if the data NOT an MPEG header
 */
int decode_mpeg_header(unsigned char *data,
                       mpeg_header_data *header,
                       unsigned char v1l1_flag)
{
    int status = 1;
    unsigned char has_header = 0;
    int mdbindx = 4;

    /* Search for 11 bits of 1's */
    has_header = (data[0] & 0xff) == 0xff;
    if (has_header) {
        has_header = (data[1] & 0xe0) == 0xe0;
    }
    if (!has_header) {
        status = 0;
        goto clean_exit;
    }
    
    /* Probably have a header at this point */
    
    /* MPEG version is next 2 bits */
    header->mpeg_version = (data[1] & 0x18) >> 3;

    /* MPEG layer is next 2 bits */
    header->mpeg_layer = (data[1] & 0x06) >> 1;

    if (!v1l1_flag) {
        /* 
         * When both version and layer are 3, this is MPEG 1 layer 1, which
         * is very unlikely.  Return an error at this point, because this
         * is not the start of a frame header.
         */
        if (header->mpeg_version == 3 && header->mpeg_layer == 3) {
            status = 0;
            goto clean_exit;
        }
    }

    /* Protection bit */
    header-> protection = data[1] & 0x1;

    /* bit rate index is next 4 bits */
    header->br_index = (data[2] & 0xf0) >> 4;

    /* sample rate index is next 2 bits */
    header->sr_index = (data[2] & 0x0c) >> 2;

    /* pad bit */
    header->pad = (data[2] & 0x02) >> 1;

    /* private bit */
    header->private = data[2] & 0x01;

    /* Channel mode is next 2 bits */
    header->channel_mode = (data[3] & 0xc0) >> 6;

    /* Mode extension is next 2 bits */
    header->joint_ext_mode = (data[3] & 0x30) >> 4;

    /* Copyright bit */
    header->copyright = (data[3] & 0x08) >> 3;

    /* Original bit */
    header->original = (data[3] & 0x04) >> 2;

    /* Emphasis is next 2 bits */
    header->emphasis = data[3] & 0x03;



    /* Intepret MPEG version value */
    switch(header->mpeg_version) {
      case 0:
        header->mpeg_version_index = MPEG_25;
        break;

      case 2:
        header->mpeg_version_index = MPEG_2;
        break;

      case 3:
        header->mpeg_version_index = MPEG_1;
        break;

      case 1:
      default:
        header->mpeg_version_index = MPEG_VERSION_RESERVED;

        /* Invalid value for this field; return error */
        status = 0;
        goto clean_exit;
        break;
    }

    /* Intepret MPEG layer value */
    switch(header->mpeg_layer) {

      case 1:
        header->mpeg_layer_index = 3;
        header->samples_per_frame = 1152;
        break;

      case 2:
        header->mpeg_layer_index = 2;
        header->samples_per_frame = 1152;
        break;

      case 3:
        header->mpeg_layer_index = 1;
        header->samples_per_frame = 384;
        break;

      case 0:
      default:
        header->mpeg_layer_index = -1;
        status = 0;
        goto clean_exit;
        break;
    }


    /*
     * This is an emperical hack.  Lacking good documentation in this area,
     * but using an actual MPEG 2 Layer 3 file as guidance, the correct
     * number of samples per frame is half that of MPEG 1 audio files.
     * Warning: only layer 3 is known to be correct here.  It is assumed
     * layer 2 is the same, and nothing is being assumed about layer 1.
     */
    switch(header->mpeg_version_index) {
      case MPEG_2:
      case MPEG_25:
        switch(header->mpeg_layer_index) {
          case 2:
          case 3:
            header->samples_per_frame /= 2;
        }
        break;
    }

    /* Protection bit; 0 = Protected; 1 = not protected */
    switch(header->protection) {
      case 0:
        header->protection = 1;
        break;

      case 1:
      default:
        header->protection = 0;
        break;
    }
    
    header->bit_rate = 
        get_bitrate_from_index(header->mpeg_version_index,
                               header->mpeg_layer_index, header->br_index);

    /* Bogus value, assume this is not frame header */
    if (header->bit_rate == 0) {
        status = 0;
        goto clean_exit;
    }

    /* Sample rate */
    header->sample_rate = 
        sample_rates[header->mpeg_version_index][header->sr_index];

    /* Bogus value, assume this is not frame header */
    if (header->sample_rate == 0) {
        status = 0;
        goto clean_exit;
    }

    header->frame_size = 
        mpeg_compute_frame_size(header->mpeg_version_index,
                                header->mpeg_layer_index,
                                header->bit_rate,
                                header->sample_rate,
                                header->pad);

    /* Bogus value, assume this is not frame header */
    if (header->frame_size == 0) {
        status = 0;
        goto clean_exit;
    }

#if 0
    if (header->protection) {
        mdbindx += 16;
    }
#endif
    /* 
     * Decode main_data_begin position.  Note: This cannot be done 
     * if protection is set, as the 16 bytes of CRC are in the way.
     */
    header->main_data_begin = data[mdbindx];
    header->main_data_begin <<= 1;
    header->main_data_begin |= ((data[mdbindx+1]&0x80)>>7);

clean_exit:
    return status;
}


void mpeg_header_values2str_3(mpeg_header_data *h, long filepos, char *cp)
{
    static char *channel_modes[] =
        { "Stereo",
          "Joint stereo",
          "Dual channel", 
          "Single Channel"
        };

    static char *mpeg_version[] =
        { "MPEG Version 1",
          "MPEG Version 2",
          "MPEG Version 2.5",
          "Reserved"
        };

    static char *mpeg_layer[] =
        { "Reserved",
          "Layer I",
          "Layer II",
          "Layer III"
        };

    static char *mode_ext_i_ii[] =
        { "Bands 4  to 31",
          "Bands 8  to 31",
          "Bands 12 to 31",
          "Bands 16 to 31"
        };

    static char *mode_ext_iii[] =
        { "Intensity stereo off; Mid-side stereo off",
          "Intensity stereo on;  Mid-side stereo off",
          "Intensity stereo off; Mid-side stereo on",
          "Intensity stereo on;  Mid-side stereo on"
        };

    static char *emphasis[] =
        { "None",
          "50/15 ms",
          "reserved", 
          "CCIT J.17"
        };

    cp += sprintf(cp, "File position:  %-6ld (0x%lx)\n",
                  h->position, h->position);
    cp += sprintf(cp, "Frame size:     %-6d (0x%x)\n",
                  h->frame_size, h->frame_size);
    cp += sprintf(cp, "MPEG version:   %-6d (%s)\n",
                  h->mpeg_version, mpeg_version[h->mpeg_version_index%4]);
    cp += sprintf(cp, "MPEG layer:     %-6d (%s)\n",
                  h->mpeg_layer, mpeg_layer[h->mpeg_layer_index%4]);
    cp += sprintf(cp, "Protection:     %-6d (%s)\n",
                  h->protection, h->protection ? "yes" : "no");
    cp += sprintf(cp, "bitrate:        %-6d (%d)\n",
                  h->br_index, h->bit_rate);
    cp += sprintf(cp, "sample_rate:    %-6d (%d)\n",
                  h->sr_index, h->sample_rate);
    cp += sprintf(cp, "Pad:            %-6d (%s)\n",
                  h->pad,  h->pad ? "yes" : "no");
    cp += sprintf(cp, "Private:        %-6d (%s)\n",
                  h->private, h->private ? "yes" : "no");
    cp += sprintf(cp, "channel_mode:   %-6d (%s)\n",
                  h->channel_mode, channel_modes[h->channel_mode%4]);
    if (h->mpeg_layer_index == 1 || h->mpeg_layer_index == 2) {
        cp += sprintf(cp, "joint_ext_mode: %-6d (%s)\n",
                      h->joint_ext_mode, mode_ext_i_ii[h->joint_ext_mode%4]);
    }
    else if (h->mpeg_layer_index == 3 && h->channel_mode == 1) {
        cp += sprintf(cp, "joint_ext_mode: %-6d (%s)\n",
                      h->joint_ext_mode, mode_ext_iii[h->joint_ext_mode%4]);
    }
    else {
        cp += sprintf(cp, "joint_ext_mode: %d\n",        h->joint_ext_mode);
    }
    cp += sprintf(cp, "Copyright:      %-6d (%s)\n",     h->copyright,
                  h->copyright ? "yes" : "no");
    cp += sprintf(cp, "Original:       %-6d (%s)\n",     h->original, 
                  h->original ? "yes" : "no");
    cp += sprintf(cp, "Emphasis:       %-6d (%s)\n",     h->emphasis,
                  emphasis[h->emphasis%4]);
    if (filepos == -1) {
        cp += sprintf(cp, "MainDataBegin:  %-6d\n",  h->main_data_begin);
    }
    else {
        cp += sprintf(cp, "MainDataBegin:  %-6d (%ld)\n", h->main_data_begin,
                      filepos - h->main_data_begin);
    }
    cp += sprintf(cp, "\n");
}


void mpeg_header_values2str(mpeg_header_data *h, char *cp)
{
    mpeg_header_values2str_3(h, -1, cp);
}


/*
 * Locate the beginning of an MPEG frame in a buffer.  The start of
 * the buffer is scanned for a properly encoded MPEG frame header.
 * When not found, a linear search in the buffer is started until a valid
 * frame is found.  When found, the start of the next frame is
 * searched for by skipping the current frame data.  
 *
 * Return values:  0 Frame not found
 *                 1 Frame found and next frame header found
 *                -1 Frame header found, but ran out of data 
 *                   for following frame search
 */
int find_mpeg_header_buf(unsigned char *buf,
                         int len,
                         mpeg_header_data *header,
                         unsigned char v1l1_flag)
{
    int              found = 0;
    int              sts   = 0;
    mpeg_header_data tmp_header;
    int              indx = 0;

    memset(&tmp_header, 0, sizeof(tmp_header));
    while (!sts && (indx + tmp_header.frame_size + 4 < len)) {
        sts = decode_mpeg_header(&buf[indx], &tmp_header, v1l1_flag);
        if (sts) {
            found            = sts = -sts;
            *header          = tmp_header;
            header->position = indx;

            if (indx + tmp_header.frame_size < len) {
                /*
                 * Check if next header is present at computed offset.  If it 
                 * is not, look one byte earlier.  Sometimes the pad 
                 * computation is wrong.
                 */
                sts = decode_mpeg_header(&buf[indx + tmp_header.frame_size],
                                           &tmp_header, v1l1_flag);
                if (!sts) {
                    indx++;
                }
            }
        }
        else {
            indx++;
        }
#if defined(_MPGEDIT_DEBUG)
        printf("find_mpeg_header_buf: offset=%d\n", indx);
#endif
    }

    if (found && sts) {
        found = sts;
    }

#if defined(_MPGEDIT_DEBUG)
    if (found) {
        printf("find_mpeg_header_buf: frame size=%d found=%d\n",
               header->frame_size, found);
    }
#endif

    return found;
}


/*
 * Exported API 
 */
int mpgedit_header_decode(unsigned char *data,
                          mpeg_header_data *header,
                          unsigned char v1l1_flag)
{
    return decode_mpeg_header(data,
                              header,
                              v1l1_flag);
}

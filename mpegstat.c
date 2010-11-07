/*
 * mpgedit statistics manipulation functions
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

#include <stdio.h>
#include "mp3time.h"
#include "header.h"
#include "mpegstat.h"


void mpeg_file_stats_init(mpeg_file_stats *s, char *filename,
                          int offset, int frames)
{
    memset(s, 0, sizeof(*s));
    s->min_bitrate = 1000000;
    s->max_bitrate = -1;
    s->filename    = filename;
    s->file_size   = offset;
    s->num_frames  = frames;
}


void mpeg_file_stats_join(mpeg_file_stats *global_stats, mpeg_file_stats *stat_entry)
{
    int i;
    global_stats->num_frames += stat_entry->num_frames;

    if (stat_entry->max_bitrate > global_stats->max_bitrate) {
        global_stats->max_bitrate = stat_entry->max_bitrate;
    }

    if (stat_entry->min_bitrate < global_stats->min_bitrate) {
        global_stats->min_bitrate = stat_entry->min_bitrate;
    }

    global_stats->avg_bitrate += stat_entry->avg_bitrate;
    global_stats->file_size += stat_entry->file_size;
                               
    for (i=0; i<16; i++) {
        global_stats->bitrate_frame_cnt[i] += stat_entry->bitrate_frame_cnt[i];
    }
}


void mpeg_file_stats_gather(mpeg_file_stats *stats, mpeg_header_data *header)
{
    stats->num_frames++;
    if (header->bit_rate > stats->max_bitrate) {
        stats->max_bitrate = header->bit_rate;
    }
    if (header->bit_rate < stats->min_bitrate) {
        stats->min_bitrate = header->bit_rate;
    }
    stats->avg_bitrate       += header->bit_rate;
    stats->file_size         += header->position;
    stats->file_size         += header->frame_size;
    stats->bitrate_frame_cnt[header->br_index]++;
    stats->mpeg_version_index = header->mpeg_version_index;
    stats->mpeg_layer         = header->mpeg_layer_index;
}


void mpeg_file_stats2str(mpeg_file_stats *stats,
                         mpeg_time *time,
                         char *cp)
{
    int  min;
    long sec;
    long seconds;
    long usec;
    int  i;
    int bitrate;
    long br_count;
    int percent;
    char percent_str[32];
    int is_vbr = 0;

    mpeg_time_gettime(time, &sec, &usec);
    seconds = sec;
    min = sec / 60;
    sec %= 60;

    cp += sprintf(cp, "\n\n");
    if (stats->num_frames == 0) {
        cp += sprintf(cp, 
            "File is empty, or does not contain MPEG audio data\n");
        return;
    }

    if (stats->filename) {
        cp += sprintf(cp, "File name:    %s\n", stats->filename);
    }
    if (stats->max_bitrate != stats->min_bitrate) {
        is_vbr = 1;
        cp += sprintf(cp, "VBR Min:      %d\n", stats->min_bitrate);
        cp += sprintf(cp, "VBR Max:      %d\n", stats->max_bitrate);
        cp += sprintf(cp, "VBR Average:  %ld\n", 
                      stats->avg_bitrate / stats->num_frames);
    }
    else {
        cp += sprintf(cp, "CBR:          %d\n", stats->max_bitrate);
    }
    cp += sprintf(cp, "Total frames: %ld\n", stats->num_frames);
    cp += sprintf(cp, "File size:    %ld\n", stats->file_size);
    cp += sprintf(cp, "Track length: %d:%.2d.%03d (%lds)\n",
           (int) min, (int) sec, (int) (usec / 1000), seconds);

    if (is_vbr) {
        cp += sprintf(cp, "\nVariable Bit Rate frame statistics\n");
        cp += sprintf(cp, "Bit Rate      Frame Count\n");
        cp += sprintf(cp, "-------------------------\n");
        for (i=0; i<16; i++) {
            br_count = stats->bitrate_frame_cnt[i];
            if (br_count > 0) {
                bitrate = 
                    get_bitrate_from_index(stats->mpeg_version_index,
                                           stats->mpeg_layer,
                                           i);
                percent = br_count * 100 / stats->num_frames;
                if (percent == 0) {
                    sprintf(percent_str, "(<1%%)");
                }
                else {
                    sprintf(percent_str, "(%d%%)", percent);
                }
                cp += sprintf(cp, "%3d kbps:     %6ld %s\n",
                       bitrate, br_count, percent_str);
                    
            }
        }
    }
    cp += sprintf(cp, "\n\n");
}


int mpeg_file_stats_is_vbr(mpeg_file_stats *stats)
{
    if (!stats) {
        return -1;
    }
    return stats->max_bitrate != stats->min_bitrate;
}

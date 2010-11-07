/*
 * mpgedit file plugin API header
 *
 * Copyright (C) 2001-2006 Adam Bernstein.
 * All Rights Reserved.
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

#ifndef _PLUGIN_H
#define _PLUGIN_H
#include <stdio.h>


typedef void (*void_fp_t)(void);
typedef void *(*mpgdecoder_alloc_t)(void);
typedef void (*mpgdecoder_free_t)(void *);
typedef void (*mpgdecoder_init_t)(void *);
typedef FILE *(*mpgdecoder_open_t)(void *, int, int);
typedef void (*mpgdecoder_close_t)(void *);
typedef void (*mpgdecoder_play_frame_t)
         (void *, FILE **, 
          unsigned char *, int, 
          int, int);
typedef void (*mpgdecoder_play_skip_frame_t)
         (void *, FILE **, 
          unsigned char *, int, 
          unsigned char *, int, 
          int, int, int *);
typedef char *(*mpgdecoder_api_ver_06_t)(void);
typedef int (*mpgdecoder_decode_frame_t) 
        (void *,
         unsigned char *, int ,
         unsigned char *, int ,
         unsigned char **, int *, int*);
typedef int (*mpgdecoder_reset_audio_t)(void *);
typedef int (*mpgdecoder_volume_get_t)(void *, int *, int *);
typedef int (*mpgdecoder_volume_set_t)(void *, int,  int);
typedef void *(*mpgdecoder_new_t) (void *, int, int, int, int);

/*
 * This union is needed to allow dlsym() assignment  without
 * casting required, and to preserve type checking.  dlsym() 
 * always assigns to voidcast entry.
 */
typedef union _plugin_fps {
    void_fp_t               voidcast;
    mpgdecoder_alloc_t      alloc;
    mpgdecoder_free_t       free;
    mpgdecoder_init_t       init;
    mpgdecoder_open_t       open;
    mpgdecoder_close_t      close;
    mpgdecoder_play_frame_t play;
    mpgdecoder_api_ver_06_t version;
    mpgdecoder_decode_frame_t decode;
    mpgdecoder_play_skip_frame_t play_skip;
    mpgdecoder_reset_audio_t reset_audio;
    mpgdecoder_volume_get_t volume_get;
    mpgdecoder_volume_set_t volume_set;
    mpgdecoder_new_t        new;
} plugin_fps;


typedef struct _plugin_fcn_desc {
    char       *name;
    plugin_fps func;
} plugin_fcn_dsc;


/*
 * These definitions must always be in the same order as the 
 * function names when initialized in mpgfio_defaults_decoder_plugin()
 */
#define fp_mpgdecoder_alloc       (fp_mpgdecoder[0].func.alloc)
#define fp_mpgdecoder_free        (fp_mpgdecoder[1].func.free)
#define fp_mpgdecoder_init        (fp_mpgdecoder[2].func.init)
#define fp_mpgdecoder_open        (fp_mpgdecoder[3].func.open)
#define fp_mpgdecoder_close       (fp_mpgdecoder[4].func.close)
#define fp_mpgdecoder_play_frame  (fp_mpgdecoder[5].func.play)
#define fp_mpgdecoder_api_ver_06  (fp_mpgdecoder[6].func.version)

/* Extension functions */
#define fp_mpgdecoder_decode_frame \
                                  (fp_mpgdecoder[7].func.decode)
#define fp_mpgdecoder_play_skip_frame \
                                  (fp_mpgdecoder[8].func.play_skip)
#define fp_mpgdecoder_reset_audio (fp_mpgdecoder[9].func.reset_audio)
#define fp_mpgdecoder_volume_get  (fp_mpgdecoder[10].func.volume_get)
#define fp_mpgdecoder_volume_set  (fp_mpgdecoder[11].func.volume_set)
#define fp_mpgdecoder_new         (fp_mpgdecoder[12].func.new)

plugin_fcn_dsc *mpgfio_alloc_decoder_plugin(void);
void mpgfio_defaults_decoder_plugin(plugin_fcn_dsc *f);
char *mpegfio_load_decoder_plugin(char *file, plugin_fcn_dsc *f);

char *mpegfio_load_decoder_plugin3(char *file,
                                   plugin_fcn_dsc *f,
                                   char **pathlist);

#endif


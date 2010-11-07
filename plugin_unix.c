/*
 * mpgedit file plugin API
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

#include "portability.h"
#include "decoder.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "plugin.h"



void mpgfio_defaults_decoder_plugin(plugin_fcn_dsc *f)
{
    f[0].name            = "mpgdecoder_alloc";
    f[1].name            = "mpgdecoder_free";
    f[2].name            = "mpgdecoder_init";
    f[3].name            = "mpgdecoder_open";
    f[4].name            = "mpgdecoder_close";
    f[5].name            = "mpgdecoder_play_frame";
    f[6].name            = "mpgdecoder_version_06";

    f[0].func.alloc      = default_mpgdecoder_alloc;
    f[1].func.free       = default_mpgdecoder_free;
    f[2].func.init       = default_mpgdecoder_init;
    f[3].func.open       = default_mpgdecoder_open;
    f[4].func.close      = default_mpgdecoder_close;
    f[5].func.play       = default_mpgdecoder_play_frame;
    f[6].func.version    = default_mpgdecoder_version_06;
}


/* Add new plugin functions here */
#define MPGDECODER_EXT_NUM_FPS 14

void mpgfio_defaults_decoder_extensions(plugin_fcn_dsc *f)
{
    int i;

    i=7;
    f[i].name            = "mpgdecoder_decode_frame";
    f[i++].func.decode   = default_mpgdecoder_decode_frame;
    f[i].name            = "mpgdecoder_play_skip_frame";
    f[i++].func.play_skip = default_mpgdecoder_play_skip_frame;
    f[i].name            = "mpgdecoder_reset_audio";
    f[i++].func.reset_audio = default_mpgdecoder_reset_audio;
    f[i].name              = "mpgdecoder_volume_get";
    f[i++].func.volume_get = default_mpgdecoder_volume_get;
    f[i].name              = "mpgdecoder_volume_set";
    f[i++].func.volume_set = default_mpgdecoder_volume_set;
    f[i].name              = "mpgdecoder_new";
    f[i++].func.new        = default_mpgdecoder_new;

    /*
     * Always NULL terminate this list! Otherwise bad things happen.
     */
    f[i].name            = NULL;
    f[i++].func.voidcast = NULL;
}


/*
 * This value must be 2 greater than the last index offset above.
 * +1 because it is zero based above, but this is a a size,
 * and +1 for the NULL entry at the end of the list
 */
#define MPGDECODER_NUM_FPS  (2 + MPGDECODER_EXT_NUM_FPS)


#if defined(WIN32)
  
char *mpgfio_dlerror(void)
{
    void *msg;

    FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM | 
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        0,
        (LPTSTR) &msg,
        0,
        NULL 
    );
    return (char *) msg;
}

void *mpgfio_dlopen(const char *filename)
{
    return LoadLibrary(filename);
}


void *mpgfio_dlsym(void *handle, char *symbol)
{
    return GetProcAddress(handle, symbol);
}

#elif defined(__hpux)

char *mpgfio_dlerror(void)
{
    char *errmsg;

    if (errno != 0) {
        errmsg = strdup(strerror(errno));
    }
    else {
        errmsg = strdup("unknown error");
    }
    return errmsg;
}


void *mpgfio_dlopen(const char *filename)
{
    shl_t *h;

    h = calloc(1, sizeof(shl_t));
    if (!h) {
        return NULL;
    }
    *h = shl_load(filename, BIND_DEFERRED | DYNAMIC_PATH, 0L);
    return (void *) h;
}


void *mpgfio_dlsym(void *handle, char *symbol)
{
    void *fp = NULL;
    int sts;

    sts = shl_findsym(handle, symbol, TYPE_UNDEFINED, (void*)(&fp));
    return sts == -1 ? NULL : fp;
}

#else

char *mpgfio_dlerror(void)
{
    char *errmsg = strdup((char *) dlerror());

    if (!errmsg) {
        errmsg = strdup("dlerror: unknown error");
    }
    return errmsg;
}

void *mpgfio_dlopen(const char *filename)
{
    return dlopen(filename, RTLD_LAZY);
}


void *mpgfio_dlsym(void *handle, char *symbol)
{
    return dlsym(handle, symbol);
}


#endif


plugin_fcn_dsc *mpgfio_alloc_decoder_plugin(void)
{
    plugin_fcn_dsc *f;

    f = (plugin_fcn_dsc *) calloc(MPGDECODER_NUM_FPS, sizeof(plugin_fcn_dsc));
    if (!f) {
        return f;
    }
    mpgfio_defaults_decoder_plugin(f);
    return f;
}


/*
 * Load extension functions available since 0.7 release
 */
void mpgfio_load_extensions(void *ctx, plugin_fcn_dsc *fp_mpgdecoder, int i)
{
    void_fp_t tmpfp;

    mpgfio_defaults_decoder_extensions(fp_mpgdecoder);

    /*
     * Try to load extension functions. If none are present, the defaults
     * are used.  The defaults return not implemented.
     */
    while (fp_mpgdecoder[i].name) {
        tmpfp = (void_fp_t) mpgfio_dlsym(ctx, fp_mpgdecoder[i].name);
        if (tmpfp) {
            fp_mpgdecoder[i].func.voidcast = tmpfp;
        }
        i++;
    }
}


/*
 * Portability issue here.  This is UNIX (SYSV style) dynamic library
 * load.  This must be re-written to call LoadLibraryEx() and
 * GetProcAddress() on Windows.  Some UNIX systems (HP-UX) have a 
 * different API for dynamically loading shared libraries; shl_load()
 * and shl_findsym() must be used.
 */
char *mpegfio_load_decoder_plugin(char *file, plugin_fcn_dsc *fp_mpgdecoder)
{
    void *ctx;
    char *libname = NULL;
    int sts = 1;
    char *nameptr;
    int i;
    void_fp_t tmpfp;
    char *errmsg = NULL;

    /*
     * Build a shared library name string.  Allocate enough space where
     * a relative path prefix and a shared library extension can be added.
     * .dll is the worst case length for all systems; this is only .dll
     * for Windows, .so for most UNIX and .sl for HP-UX.
     */
    libname = (char *) malloc(sizeof("./") + strlen(file) + 
                              sizeof(_SHLIB_FILE_EXT));
    if (!libname) {
        goto clean_exit;
    }

    /*
     * Always build library path with relative extension.  This is not
     * always used.  Use nameptr to point at the start of the name passed in. 
     */
    strcpy(libname, "./");
    strcat(libname, file);
    nameptr = libname+2;
    if (!strrchr(nameptr, '.')) {
        strcat(libname, _SHLIB_FILE_EXT);
    }

    ctx = mpgfio_dlopen(nameptr);
    if (!ctx) {
        if (nameptr[0] != '/') {
            ctx = mpgfio_dlopen(libname);
        }
    }
    if (!ctx) {
        errmsg = strdup(mpgfio_dlerror());
        goto clean_exit;
    }


    for (i=0; fp_mpgdecoder[i].name; i++) {
        tmpfp = (void_fp_t) mpgfio_dlsym(ctx, fp_mpgdecoder[i].name);
        if (!tmpfp) {
            errmsg = strdup(mpgfio_dlerror());
            goto clean_exit;
        }
        fp_mpgdecoder[i].func.voidcast = tmpfp;
    }

    mpgfio_load_extensions(ctx, fp_mpgdecoder, i);
    

    sts = 0;
clean_exit:
    if (libname) {
        free(libname);
    }
    if (sts) {
        mpgfio_defaults_decoder_plugin(fp_mpgdecoder);
    }
    return errmsg;
}


char *mpegfio_load_decoder_plugin3(char *file,
                                   plugin_fcn_dsc *fp_mpgdecoder,
                                   char **pathlist)
{
    char *errmsg = NULL;
    char *dir;
    char *path;

    /* 
     * Try filename first. When LD_LIBRARY_PATH is set, and only a library
     * name is provided, this will work.
     */
    errmsg = mpegfio_load_decoder_plugin(file, fp_mpgdecoder);
    if (!errmsg) {
        return NULL;
    }

    while (*pathlist) {
        dir = *pathlist;
        path = (char *) malloc(strlen(dir) + strlen(file) + 2);
        if (path) {
            strcpy(path, dir);
            strcat(path, "/");
            strcat(path, file);
        }
        errmsg = mpegfio_load_decoder_plugin(path, fp_mpgdecoder);
        free(path);
        if (!errmsg) {
            return NULL;
        }
        pathlist++;
    }
    return errmsg;
}

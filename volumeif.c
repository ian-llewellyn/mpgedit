/*
 * mpgedit volume control interface
 *
 * Copyright (C) 2003-2006 Adam Bernstein
 * All rights reserved.
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
#include <fcntl.h>
#include "volumeif.h"

typedef struct _volume_control_handle {
    int          fd;     /* UNIX fd/Win32 handle to mixer device */
    unsigned int id;     /* Win32 volume control ID              */
} volume_control_handle;


#if defined(WIN32)
/*
 * These functions are called from audio_win32.c.
 */

/*
 * Win32 implementation.  Tested on NT4 and W2K.
 */
#include "wmixer.h"

void mpgedit_volume_set(void *fd, int lvol, int rvol)
{
    volume_control_handle *h = (volume_control_handle *) fd;
    /* Somehow use lvol and rvol both */

    /* Multiply by 655 to get maximum volume for 100 */
    lvol *= WIN32_MIXER_SCALE_FACTOR;
    set_control_details(h->fd, h->id, lvol);
}


int mpgedit_volume_get(void *fd, int *lvol, int *rvol)
{
    int                   volume;
    volume_control_handle *h = (volume_control_handle *) fd;

    volume = get_control_details(h->fd, h->id);
    if (volume != -1) {
        *lvol = *rvol = volume / WIN32_MIXER_SCALE_FACTOR;
    }
    return volume;
}


void *mpgedit_volume_open(void)
{
    volume_control_handle *h;

    h = (volume_control_handle *) calloc(1, sizeof(volume_control_handle));
    if (!h) {
        return NULL;
    }
    h->id = open_mixer_pcm_volume_control(&h->fd);

    return h;
}


void mpgedit_volume_close(void *ctx)
{
    volume_control_handle *h;

    if (!ctx) {
        return;
    }

    h = (volume_control_handle *) ctx;
    close_mixer_control(h->fd);
    free(h);
}


#elif defined(__linux)
#if 0
/* This is no longer used.  The audio mixer is now in audio_oss.c
 * Part of the implementation was already in audio_oss.c audio_open().
 * Additional functions from this * module [audio_set_gain() and
 * audio_get_gain()] were added to audio_oss.c.
 */

/*
 * OSS implementation.  Only tested on Linux.
 */

#endif

#else

void mpgedit_volume_set(void *fd, int lvol, int rvol)
{
}


int mpgedit_volume_get(void *fd, int *lvol, int *rvol)
{
    *lvol = *rvol = 0;
    return -1;
}



void *mpgedit_volume_open(void)
{
    return NULL;
}



void mpgedit_volume_close(void *ctx)
{
}



#endif

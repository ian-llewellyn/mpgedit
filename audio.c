/*
 * soundcard initialization function for decoder_mpg123 playback plugin
 *
 * Copyright (C) 2002 Adam Bernstein
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
 * This module does not actually implement the interface to the soundcard.
 * It does includes the platform dependent soundcard driver source file,
 * based on standard compiler macro definitions.  This technique is
 * often not completely reliable, but should work fine for the currently
 * supported platforms.
 *
 * The audio_xxx.c files are copied directly from the mpg123-0.59r 
 * project,  written by: Michael Hipp (www.mpg123.de).  This implementation
 * now replaces the original dsp.c for Linux, utilizing the OSS sound API.
 * Since all of the porting work has already been done by the mpg123 project
 * in this area, it makes the most sense to leverage this work here.
 *
 * The technique used in this file is to first determine which
 * platform we are based upon standard compiler pre-processor defines.
 * Once the platform type is known, any variables and defines needed to
 * compile the platform specific audio module are declared.  After that,
 * the platform specific source code is included.
 */

#ifndef lint
static char SccsId[] = "$Id: audio.c,v 1.4.2.1 2008/07/13 21:58:12 number6 Exp $";
#endif

#if defined(__linux)
#define LINUX
int outburst = 0;
#if defined(ALSA)
#include "audio_alsa.c"
#else
#include "audio_oss.c"
#endif

/*
 * Stub that calls audio_get_parameters() to silence compiler warning
 */
void __stubcall_audio_get_parameters(void)
{
#if 0
    audio_get_parameters(0);
#endif

}
#elif defined(__sun)
#  if defined(__sparc) && defined(__SVR4)
#    define SOLARIS
#  endif
#include "audio_sun.c"
struct parameter param;

#elif defined(__hpux)
#include "audio_hp.c"

#elif defined(_AIX)
#include "audio_aix.c"

#elif defined(WIN32)
#include "audio_win32.c"

#elif defined(__macosx)
#include "audio_macosx.c"

#else
%%% Not ported to your operating system %%%
#endif

void *GetVbrTag(void *x, void *y)
{
    return 0;
}

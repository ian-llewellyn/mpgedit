/*
 * mpgedit portability abstraction header
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

#ifndef _PORTABILITY_H
#define _PORTABILITY_H

#if !defined(_WIN32) 

/* UNIX Section */
#define VOLUME_DEFAULT_VALUE -1

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#if defined(__hpux)
#include <dl.h>
#define  _SHLIB_FILE_EXT ".sl"
#else
#include <dlfcn.h>
#endif
#if defined(__macosx)
#define VOLUME_DEFAULT_VALUE 20

#define  _SHLIB_FILE_EXT ".dylib"
#else
#define  _SHLIB_FILE_EXT ".so"
#endif
#define _DIR_SEPARATOR_CHAR '/'
#define _DIR_SEPARATOR_CHAR_NOT '\\'
#define _DSOEXPORT
#define _CDECL
#define _MAIN(x)

#define set_stdout_binary()
#define _SET_X11_DISPLAY_NAME() if (!getenv("DISPLAY")) putenv("DISPLAY=:0")

#else

/* WIN32 Section */
#define VOLUME_DEFAULT_VALUE -1

#include <windows.h>
#include <io.h>
#include "win32/getopt.h"
#include <fcntl.h>
#define  _SHLIB_FILE_EXT ".dll"

/* Why are these defines needed, but others, like strdup and fopen are not ?? */
#ifndef access
#define access _access
#endif
#ifndef sleep
#define sleep(z)  _sleep((z)*1000)
#endif

/* Defines needed for access() */
#ifndef F_OK
#define F_OK 00
#endif
#ifndef W_OK
#define W_OK 02
#endif
#ifndef R_OK
#define R_OK 04
#endif
#ifndef S_ISREG
#define S_ISREG(x) (((x) & _S_IFREG) == _S_IFREG)
#endif
#define _DIR_SEPARATOR_CHAR '\\'
#define _DIR_SEPARATOR_CHAR_NOT '/'
#define _DSOEXPORT __declspec(dllexport)
#define _CDECL     __cdecl
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

typedef unsigned long int uint32_t;
typedef          long int int32_t;

#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

#ifdef _DEBUG
# define _MAIN(x)
#else
# include <stdlib.h>
# define _MAIN(prog) \
int APIENTRY WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int showhow) \
{ return (prog)(__argc, __argv); }
#endif

#define set_stdout_binary() \
do { _setmode (_fileno(stdout), O_BINARY);  \
     _setmode (_fileno(stdin),  O_BINARY);  \
} while (0)

#define _SET_X11_DISPLAY_NAME()
#endif

#ifdef _USE_CURSES_H
#if defined(__linux)
#include <ncurses.h>
#else
#include <curses.h>
#endif
#endif /* _USE_CURSES_H */

#ifdef _DEBUG
#define D_printf(v) printf v
#else
#define D_printf(v)
#endif

#endif /* _PORTABILITY_H */

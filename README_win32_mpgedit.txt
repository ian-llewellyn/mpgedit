     mpgedit - MPEG 1/2/2.5 (MP3) audio file editor
               Version 0.74beta1 January 2009
                     http://mpgedit.org
        Copyright (C) 2001-2009 by Adam Bernstein


Written by: Adam Bernstein <number6@mpgedit.org>, released under
the GPL.  See "COPYING" for full details.

    ------------------------------------------------------------------------


BACKGROUND
----------
This is the Windows installation package of mpgedit, the command line
and curses version of mpgedit. This is release 0.74beta1.

The major feature of this release is the graphical interface version of
mpgedit.  Details of the major features included in this release are listed
in the NEWS file.  This product is available for Linux and Windows at this
time.  The Windows version is known to function on Windows 98->Windows XP.
The Linux implementation is built on RedHat 6.2 and RedHat 7.1 systems, and
should function on RedHat 6.x, 7.x and 8.x systems.



INSTALLATION
------------
This package installs xmpgedit by default into this directory:

    \Program Files\mpgedit.org\mpgedit

On Windows NT/W2K/XP, you must install mpgedit with a user that has
system Administrator privilege.  The installer will prompt you for a
user with Administrator privilege when you attempt to install with a
user that does not.  After you enter the username and password for the
Administrator, the installation will proceed.  Otherwise, you will need
to log off the current user and log in as Administrator, then attempt the
installation again.

The PATH environment variable is updated during the install.  Before this
change is visable to cmd.exe, you must log off and log on again.
After all, this is Windows.  At least you don't have to reboot.  ;-}



UNINSTALLATION
--------------
Uninstall mpgedit with the Windows "Add/Remove Programs" application:

    Settings->Control Panel->Add/Remove Programs.

Remove the mpgedit 0.7beta package.



SOURCE CODE
-----------
CVS:

cvs -d \
  :pserver:anonymous@mpgedit.org:/usr/local/cvsroot login

       There is no password, just press Enter

cvs -z3 -d \
  :pserver:anonymous@mpgedit.org:/usr/local/cvsroot \
  co -r mpgedit_0-74beta1 mpgedit



COMPILING
---------
You must have Visual C++ configured for command line builds.  The
easiest way to accomplish this is run vcvars32.bat from a cmd shell.

Download the source:
    http://mpgedit.org/mpgedit/mpgedit_0-74beta1_src.tar.gz

Or download the source from CVS for 'latest'.

In a directory at the same level as the mpgedit source,
download the PDCurses 2.4 package and unzip it in a directory
named 'pdcurses'.

The build of xmpgedit is dependent on GTK+ 2.0 for Windows.  This
can be downloaded from:

  http://www.gimp.org/~tml/gimp/win32/downloads.html


These packages are needed to build:
  atk-dev-1.0.3-20020821.zip
  glib-dev-2.2.1.zip
  gtk+-dev-2.2.1.zip
  libiconv-1.8-w32-1.bin.zip
  pango-dev-1.2.1.zip

The makefile for xmpgedit expects these to be installed in:

  C:\win32\tools\gtk

The makefile must be adjusted if these packages are installed elsewhere.

You should have a directory structure something like this:

    c:\wrk\mpgedit
    c:\wrk\pdcurses
    c:\win32\tools\gtk

From a cmd shell, cd into wrk\mpgedit, and run win32_make.bat.

c:\mpgedit> win32_make.bat


DOCUMENTATION
-------------
HTML versions of the mpgedit man pages are available on the mpgedit
home page:

    http://mpgedit.org/mpgedit/docs.html



COPYRIGHT
---------
     mpgedit - MPEG 1/2/2.5 (MP3) audio file editor
               Version 0.74beta1 January 2009
                   http://mpgedit.org
        Copyright (C) 2001-2009 by Adam Bernstein


Written by: Adam Bernstein <number6@mpgedit.org>, released under
the GPL.  See "COPYING" for full details.

The mpgedit audio playback plugins uses the MPGLIB decoding engine,
from the MPG123 package, written by: Michael Hipp (www.mpg123.de)
MPGLIB is released under the GPL.

The version of MPGLIB used in this release comes from LAME 3.90 alpha 7,
released under the GNU LESSER PUBLIC LICENSE.

Copyrights (c) 1999,2000,2001 by Mark Taylor: LAME
Copyrights (c) 1998 by Michael Cheng: LAME
Copyrights (c) 1995,1996,1997 by Michael Hipp: mpglib

curses.dll comes from the PDCurses 2.4 package (pdc24_vc_w32.zip),
http://pdcurses.sourceforge.net/index.html

The Gimp Toolkit (GTK) is used by xmpgedit, http://gtk.org.

The Win32 installer uses Inno Setup (IS), Copyright (C) 1997-2003++
Jordan Russell (http://www.jrsoftware.org/isinfo.php)

The MD5 checksum library (http://sourceforge.net/projects/libmd5-rfc/)
is Copyright (C) 1999, 2000, 2002 Aladdin Enterprises.
All rights reserved.

The libdecoder_mad.so playback plugin is implemented using libmad,
libmad - MPEG audio decoder library, Underbit Technologies, Inc.
Copyright (C)  2000-2004 Underbit Technologies, Inc.
http://www.underbit.com/products/mad/

The getopt.c win32/getopt.h sources are originally from GNU getopt,
Copyright (C) 1987, 88, 89, 90, 91, 92, 1993, Free Software Foundation, Inc.

audio_macosx.c: originally written by Guillaume Outters. AudioUnit version
by Steven A. Kortze <skortze@sourceforge.net>

audio_aix.c: Code by Juergen Schoew <juergen.schoew@unix-ag.org>;
Tomas Oegren <stric@acc.umu.se>, (code for audio_close delay);
Cleanups and testing by Niklas Edmundsson <nikke@ing.umu.se>.

audio_hp.c, audio_oss.c, audio_win32.c, audio_sun.c: No attributions
present in source code.  Originally part of mpg123 project, copyrights
(c) 1995,1996,1997 by Michael Hipp.

As well as additional copyrights as documented in the source code.

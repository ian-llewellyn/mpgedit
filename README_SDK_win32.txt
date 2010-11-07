This is the README for the mpgedit SDK for Windows.  The primary purpose of
this package is to provide runtime support of py_mpgedit.  Developers could
use this SDK for development of MP3 editing applications in C, however this
practice is discouraged with this version.  The reason for this is the C API
present in this package is not considered public at this time.  That means
it can, and probably will, change at any time.  Any applications written
with the C API will probably break when built and run using later versions
of the mpgedit SDK.  You have been warned.

The python py_mpgedit extension is a public API and its use for development
is encouraged.  There are two sample programs provided in this release,
simple_play.py and simple_edit.py, in addition to the python extension
documentation.  See http://mpgedit.org/mpgedit/sdk/py_mpgedit.htm for the
complete py_mpgedit API documentation.



INSTALLATION
------------

This package installs the mpgedit SDK by default into this directory:

    \Program Files\mpgedit.org\mpgedit_sdk

On Windows NT/W2K/XP, you must install xmpgedit with a user that has
system Administrator privilege.  The installer will prompt you for a
user with Administrator privilege when you attempt to install with a
user that does not.  After you enter the username and password for the
Administrator, the installation will proceed.  Otherwise, you will need
to log off the current user and log in as Administrator, then attempt the
installation again.

The system PATH environment variable is updated during the install, making
the py_mpgedit SDK available to all users on this system.  Before this
change is visable to cmd.exe, you must log off and log on again.  After all,
this is Windows; at least you don't have to reboot.  ;-}



UNINSTALLATION
--------------
Uninstall xmpgedit with the Windows "Add/Remove Programs" application:

    Settings->Control Panel->Add/Remove Programs.

Remove the mpgedit_sdk 0.74beta package, and python2 py_mpgedit-0.3beta1
package.



TESTING
-------

Run the simple_play.py application located in the examples directory.

  cd \program files\mpgedit.org\mpgedit_sdk\examples
  simple_play.py test1.mp3

The mp3 file should play.

The simple_edit.py application cuts test1.mp3 into a number of segments, 
then joins the segments into a new output file named editif_test2.mp3.  

Run the following commands to test simple_edit.py:

  cd \program files\mpgedit.org\mpgedit_sdk\examples
  simple_edit.py test1.mp3

The contents of editif_test2.mp3 are identical to test1.mp3 when this test
is successful.  The intermediate output file editif_test1.mp3 contains the
reordered edits, and was then edited to generate editif_test2.mp3.



COPYRIGHT
---------


     mpgedit - MPEG 1/2/2.5 (MP3) audio file editor
               Version 0.74 beta January 2009
                   http://mpgedit.org
        Copyright (C) 2001-2009 by Adam Bernstein


Written by: Adam Bernstein <number6@mpgedit.org>, released under
the GPL.  See "COPYING" for full details.

Python mpgedit extension (py_mpgedit) written by Bryan Weingarten
<bryan.weingarten@mpgedit.org>.  Copyright (C) 2009, Bryan Weingarten.

The py_mpgedit python/C API interface is developed using Pyrex,
written by Greg Ewing. http://www.cosc.canterbury.ac.nz/~greg/python/Pyrex/.

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

As well as additional copyrights as documented in the source code.

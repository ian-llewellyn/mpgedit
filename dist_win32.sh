#!/bin/sh
if [ -n "$1" ]; then
  dist_dir="../../mpgedit_${1}_win32"
else
  dist_dir="../../mpgedit_0.6_win32"
fi
if [ -d $dist_dir ]; then
  echo "Distribution directory '$dist_dir' already exists"
  echo "    Remove and run this script again to regenerate"
  exit 1
fi

doc="README NEWS"
html1="html/decoder_so.html html/mp3decoder_sh.html html/mpgedit.html
       html/scramble_times_pl.html html/scramble_pl.html 
       html/unscramble_pl.html"
lib="../pdcurses/curses.dll libmpgedit_decoder.dll mpgedit.dll"
bin="mpgedit.exe mpgedit_nocurses.exe mp3_vbrpatch.exe 
     test1.pl test2.pl test3.pl test4.pl
     mpgtests.pm scramble_times.pl scramble.pl unscramble.pl"
mp3="test1.mp3 test2.mp3 ATrain23.mp3  mpg1_layer1_fl1.mp3 xingheader.mp3
     mpg1_layer1_fl7.mp3  mpg1_layer2_fl11.mp3  mpg1_layer2_fl16.mp3  
     mpg2_layer2_test24.mp3"
data="test1.split test1.join random_prefix zero_prefix ones_data test2.tag"

uid=`id | awk '{print $1}' | sed -e 's/uid=//' -e 's/(.*//'`
if [ $uid -ne 0 ]; then
  echo "you must be root to execute this script"
  exit 1
fi

[ -d $dist_dir ] || mkdir $dist_dir
cp $doc       $dist_dir
cp $html1     $dist_dir
cp $lib       $dist_dir
cp $bin       $dist_dir
cp $mp3       $dist_dir
cp $data      $dist_dir

cat << NNNN > $dist_dir/README.win32
This is the binary tarball of mpgedit 0.7 beta for Windows. This product has
been built using Microsoft Visual Studio 6.0. This build is known to
function on Windows 95 through Windows XP.  Please let me know about your
success/failure with running this code.

The release notes for this product are in "NEWS".


INSTALLATION
------------
This package has no installer.  Look at the Windows installer packages
for mpgedit and xmpgedit before considering placing any effort into
installing this package.

This package is the raw binary files distribution.  There is no "installer"
per se.  You are responsible for placing these files in the correct location,
setting PATH and the registry to properly find the executables on your system.

Unzip this package, and run from the current directory.  Your best bet is to
copy into some location like your PATH environment variable.


UNINSTALLATION
--------------
This is up to you, since the installation was done by you.


DOCUMENTATION
-------------
HTML versions of the mpgedit man pages are available on the mpgedit
home page:

    http://mpgedit.org/mpgedit/mpgedit.html


FILE LIST
---------
                                                       README.win32
NNNN
(cd $dist_dir; ls -l | sed -e '/^total/d' -e '/README.win32/d' >> README.win32)

#-rwxr--r--   1 number6  slocate    121344 Feb 27 00:03 curses.dll*
#-rwxr--r--   1 number6  slocate    198268 Feb 27 00:03 libmpgedit_decoder.dll*
#-rwxr--r--   1 number6  slocate    235320 Feb 27 00:03 mpgedit.exe*
#-rwxr--r--   1 number6  slocate    189204 Feb 27 00:03 mpgedit_nocurses.exe*
#-rw-r--r--   1 number6  slocate    596332 Feb 27 00:04 test1.mp3

cat << NNNN >> $dist_dir/README.win32

      MD5 Checksum                    File Name
      ------------                    ---------
                                  README.win32
NNNN
(cd $dist_dir; md5sum * | sed -e '/README.win32/d' >> README.win32)

#d5a933ee905e667580b25887645f891f  curses.dll
#fe1b111eea56e98703fda3a3b19c30a9  libmpgedit_decoder.dll
#74dec3f9ad34bb787c586a346386292e  mpgedit.exe
#2a55264f1792fca519da4a5fa5531957  mpgedit_nocurses.exe
#8bf58c3868a8bc51f38bc77dab649983  test1.mp3

cat << NNNN >> $dist_dir/README.win32


TESTING
-------
The current regression test is "test1.pl".  You can run the regression
test by simply executing:

UNIX:    $ ./test1.pl
Windows: C:> test1.pl

This test heavily exercises the editing features of mpgedit.  Each test
performs a series of edits that results in an output file that will be
identical to the original, unedited file.  The resultant edited file is
then compared against the original source file.

The regression test scripts are written in Perl, and run on Win32.
You must have the Perl environment installed before you can run these
tests.  Perl can be downloaded here from the ActivePerl web site:

    http://downloads.activestate.com/ActivePerl/Windows/5.6/ActivePerl-5.6.1.633
-MSWin32-x86.msi

Without Perl installed, the full regression test suite cannot be run.
However, the "slice and splice" test is available, in a fashion, for all
platforms.

The slice test is performed with the following actions:

  - Start mpgedit with no options
  - Press 'Enter' to close the help window
  - Enter the 'L' command, then load "test1.split"
  - Enter the 'E' command, save abandoned edits to file "test1.split2

The splice test is performed with the following actions:

  - Start mpgedit with no options
  - Press 'Enter' to close the help window
  - Enter the 'L' command, then load "test1.join"
  - Enter the 'n' command, entering the output file "test1_splice.mp3"
  - Enter the 'e' command, save abandoned edits to file "test1.join2"

Files test1.mp3 and test1_splice.mp3 should be identical.  This can be
verified by comparing these two files with diff:

  cmp test1.mp3 test1_splice.mp3

Cleanup after this test with the following command:

  rm -f test1.splice.mp3 test1_*.mp3 test1_*.idx test1.split2 test1.join2


SOURCE CODE
-----------
CVS:

cvs -d \
  :pserver:anonymous@mpgedit.org:/usr/local/cvsroot login

       There is no password, just press Enter

cvs -z3 -d \
  :pserver:anonymous@mpgedit.org:/usr/local/cvsroot \
  co -r latest mpgedit


COMPILING
---------
You must have Visual C++ configured for command line builds.  The
easiest way to accomplish this is run vcvars32.bat from a cmd shell.

Download the source:
    http://mpgedit.org/mpgedit/mpgedit_0-7beta_src_tgz.zip

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


COPYRIGHT
---------

NNNN
cat $dist_dir/README | \
    sed -n  -e '1,/^================/p' | sed '$,$d' >> $dist_dir/README.win32

(cd $dist_dir; chown 0 *; chgrp 0 *)
exit 0

#!/bin/sh
if [ -n "$1" ]; then
  dist_dir="../../mpgedit_${1}_linux"
else
  dist_dir="../../mpgedit_0.6_linux"
fi
if [ -d $dist_dir ]; then
  echo "Distribution directory '$dist_dir' already exists"
  echo "    Remove and run this script again to regenerate"
  exit 1
fi

doc="README NEWS"
man1="mpgedit.1 xmpgedit.1 decoder.so.1 mp3decoder.sh.1
      scramble_times.pl.1 scramble.pl.1 unscramble.pl.1"
html1="html/decoder_so.html html/mp3decoder_sh.html html/mpgedit.html 
       html/xmpgedit.html html/scramble_times_pl.html html/scramble_pl.html 
       html/unscramble_pl.html"
lib="libdecoder_mpg123.so libdecoder_mad.so libdecoder_popen.so libmpglib_mpgedit.so"
bin="mpgedit mpgedit_nocurses mp3decoder.sh mp3_vbrpatch
     test1.pl test2.pl test3.pl test4.pl mpgtests.pm 
     scramble_times.pl scramble.pl unscramble.pl
     gui/xmpgedit
     gui/blankdigit_led.xpm gui/blankpunct_led.xpm gui/close.xpm 
     gui/colon_hour_led.xpm gui/colon_led.xpm gui/colon_minute_led.xpm
     gui/eight_led.xpm gui/eject.xpm gui/five_led.xpm gui/four_led.xpm
     gui/next_t.xpm gui/nine_led.xpm gui/one_led.xpm gui/pause.xpm 
     gui/period_led.xpm gui/period_second_led.xpm gui/play.xpm gui/record.xpm
     gui/record_green.xpm gui/record_red.xpm gui/seven_led.xpm gui/six_led.xpm
     gui/stop.xpm gui/three_led.xpm gui/two_led.xpm gui/volume1.xpm 
     gui/zero_led.xpm
     mpg1_layer1_fl1.mp3  mpg1_layer1_fl7.mp3  mpg1_layer2_fl11.mp3 
     mpg1_layer2_fl16.mp3  mpg2_layer2_test24.mp3"
mp3="test1.mp3 test2.mp3 ATrain23.mp3  mpg1_layer1_fl1.mp3 xingheader.mp3
     mpg1_layer1_fl7.mp3  mpg1_layer2_fl11.mp3  mpg1_layer2_fl16.mp3
     mpg2_layer2_test24.mp3"
data="random_prefix zero_prefix ones_data test2.tag test1.split test1.join"

uid=`id | awk '{print $1}' | sed -e 's/uid=//' -e 's/(.*//'`
if [ $uid -ne 0 ]; then
  echo "you must be root to execute this script"
  exit 1
fi

[ -d $dist_dir ] || mkdir $dist_dir
cp $doc       $dist_dir
cp $man1      $dist_dir
cp $html1     $dist_dir
cp $lib       $dist_dir
cp $bin       $dist_dir
cp $mp3       $dist_dir
cp $data      $dist_dir
cp install.sh $dist_dir



cat << NNNN > $dist_dir/README.linux
This is the binary tarball of mpgedit 0.7 beta for Linux.  This product has
been built using egcs-2.91.66, with GLIBC version glibc-2.1.3-15.  This 
should function on RedHat Linux 6x and 7x systems.  You may need to install
required library components for proper operation.  mpgedit is dependent 
on these RPM packages:

    ncurses-4.2-25
    glibc-2.1.3-15

The RedHat 7.x release is dependent on these RPM packages:
   glibc-2.2.5-34
   ncurses-5.2-26
   ElectricFence-2.2.2-8 (alpha releases only)

Should this build not function for you, then download and build the 
source tarball.


INSTALLATION
------------

To install mpgedit, as root run: ./install.sh 

This script will first prompt you for the install directory, /usr/local
is the default.  Press enter to install in /usr/local.  Should
/usr/local/lib not already be in your /etc/ld.so.conf file, it will be added,
and ldconfig run.

UNINSTALLATION
--------------

To uninstall mpgedit, as root run: ./install.sh -u

This script will first prompt you for the uninstall directory, /usr/local
is the default.  Press enter to uninstall mpgedit from /usr/local.  The
/etc/ld.so.conf file is not updated by the uninstall script.

FILE LIST
---------
                                                       README.linux
NNNN

(cd $dist_dir; ls -l | sed -e '/^total/d' -e '/README.linux/d' >> README.linux)
#                                                       README
#-rw-r--r--   1 number6  slocate      3803 Jan 29 15:28 decoder.so.1
#-rwxr-xr-x   1 number6  slocate      3519 Jan 29 17:34 install.sh*
#-rwxr-xr-x   1 number6  slocate     39343 Jan 29 15:28 libdecoder_mpg123.so*
#-rwxr-xr-x   1 number6  slocate     16734 Jan 29 15:28 libdecoder_popen.so*
#-rwxr-xr-x   1 number6  slocate    139047 Jan 29 15:28 libmpglib_mpgedit.so*
#-rw-r--r--   1 number6  slocate       119 Jan 29 16:20 mp3decoder.sh
#-rw-r--r--   1 number6  slocate       868 Jan 29 15:28 mp3decoder.sh.1
#-rwxr-xr-x   1 number6  slocate    125581 Jan 29 15:28 mpgedit*
#-rw-r--r--   1 number6  slocate     23350 Jan 29 15:28 mpgedit.1
#-rwxr-xr-x   1 number6  slocate     83959 Jan 29 15:50 mpgedit_nocurses*
cat << NNNN >> $dist_dir/README.linux


      MD5 Checksum                    File Name
      ------------                    ---------
                                  README.linux
NNNN
(cd $dist_dir; md5sum * | sed -e '/README.linux/d' >> README.linux)
#                                  README
#283742550a52c1170cf6d8ffab488847  decoder.so.1
#4b40fb734e88b87528be82e6dbe72d5e  install.sh
#cec6c602641a3f7c3de9523405504c07  libdecoder_mpg123.so
#cec6c602641a3f7c3de9523405504c07  libdecoder_mad.so
#29e2e65bb91337a6be299699fe7aabb7  libdecoder_popen.so
#4598d35581699c34bbdb5ea77c049190  libmpglib_mpgedit.so
#5d8f071f4ee72210dca900b43c185166  mp3decoder.sh
#c21cbed202aa197dd394c57c3929d54c  mp3decoder.sh.1
#1dc2942ae5e00c3afbe34985f521c934  mpgedit
#a672b04104c5388f30b5eb651533ab1a  mpgedit.1
#2558a9d039181067d3fc49f41bb34632  mpgedit_nocurses
cat << NNNN >> $dist_dir/README.linux

TESTING
-------
Linux:

The current regression test is "test1.pl".  You can run the regression
test by simply executing:

$ ./test1.pl

This test heavily exercises the editing features of mpgedit.  Each test 
performs a series of edits that results in an output file that will be
identical to the original, unedited file.  The resultant edited file is
then compared against the original source file.

The regression test scripts are written in Perl, and run on Win32.
You must have the Perl environment installed before you can run these
tests.  Perl can be downloaded here from the ActivePerl web site:

    http://downloads.activestate.com/ActivePerl/Windows/5.6/ActivePerl-5.6.1.633-MSWin32-x86.msi

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



COPYRIGHT
---------


NNNN
cat $dist_dir/README | \
    sed -n  -e '1,/^================/p' | sed '$,$d' >> $dist_dir/README.linux

(cd $dist_dir; chown 0 *; chgrp 0 *)
exit 0

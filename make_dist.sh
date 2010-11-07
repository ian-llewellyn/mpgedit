#!/bin/sh
#
# Script to build production distribution of mpgedit
#
# Build directions:
# 
# 1) cd to desired build area
# 2) run make_dist.sh to fetch source, and compile for Linux
# 3) run compile for RedHat Linux 7x and Win32 platforms
# 4) su root
# 5) run make_dist.sh again to build distribution files

export PATH=$PATH:/opt/bin

. `dirname $0`/dist_parse_version.sh $0 

if [ -z "$RELEASE" ]; then
  RELEASE="0-7beta"
fi

echo VERSIONFILE=$VERSIONFILE
echo TAG=$TAG
echo RELEASE=$RELEASE
echo
sleep 2

topdir="mpgedit_$RELEASE"
if [ ! -d "$topdir" ]; then
  echo
  echo "Creating new work area '$topdir/src/mpgedit'"
  echo "    ^C now to stop this operation"
  for i in 10 9 8 7 6 5 4 3 2 1; do
    echo -n "$i "
    sleep 1
  done

  umask 002
  mkdir "$topdir"
  chmod g+ws "$topdir"
  chgrp 21 "$topdir"
  cd $topdir
  if [ ! -d src ]; then
    mkdir src
    cd src
    if [ -n "$TAG" ]; then
      cvs co -r $TAG mpgedit
    else
      cvs co mpgedit
    fi
    cvs co mpgedit_tps
    (cd mpgedit/mad/include
     ln -s ../src/mad.h mad.h)
  fi
  cd mpgedit
else
  if [ -d aix -o -d hpux10 ]; then
    echo "ERROR: You are running this script from the wrong directory"
    echo "       cwd = `pwd`"
    exit 1
  fi
  cd $topdir/src/mpgedit
fi

# pwd is now $topdir/src/mpgedit
#
dir=../..

#
# Create versioned README.txt for Win32
#
./mk_mpgedit_iss_readme.sh

if [ ! -d $dir/linux ]; then
  ./man2html.sh
  (cd $dir; mkdir linux linux_rh7 linux_fedora8 macosx solaris win32 hpux10 aix)
  (cd $dir/linux;     lndir -withrevinfo ../src)
  (cd $dir/linux_rh7; lndir -withrevinfo ../src)
  (cd $dir/linux_fedora8; lndir -withrevinfo ../src)
  (cd $dir/macosx;    lndir -withrevinfo ../src)
  (cd $dir/macosx
   mkdir -p mpgedit/gui/xmpgedit.app/Contents/MacOS/lib
   curdir=`pwd`
   cd mpgedit/gui/xmpgedit.app/Contents/MacOS/lib
   tar zxvf $curdir/mpgedit_tps/src/gtk+2-2.6.10-1_macosx.tgz
  )
  (cd $dir/solaris;   lndir -withrevinfo ../src)
  (cd $dir/win32;     lndir -withrevinfo ../src)
  (cd $dir/hpux10;    lndir -withrevinfo ../src)
  (cd $dir/aix;       lndir -withrevinfo ../src)
  (cd $dir/win32; unzip ../src/mpgedit_tps/src/pdc24_vc_w32.zip)
  (cd $dir/win32/mpgedit; ln -s ../pdcurses/curses.dll .)
  (cd $dir/win32; unzip ../src/mpgedit_tps/src/gtk-2.2.1-dll.zip)
  (cd $dir/linux/mpgedit; ./configure)
  echo "You must perform platform specific builds for win32 and solaris"
  echo "Afterwards, execute this script again"
  exit 0
fi

if [ ! -f $dir/linux/mpgedit/mpgedit -o \
     ! -f $dir/win32/mpgedit/mpgedit.exe ]; then
  
  echo "You must perform platform specific builds for win32 and solaris"
  echo "Afterwards, execute this script again"
  exit 0
fi

echo debug pwd=`pwd` dir=$dir
uid=`id -u`
if [ $uid -ne 0 ]; then
  echo "you must be root to continue executing from this point"
  exit 1
fi


DONTCOPY=1
if [ "$1" = "copyweb" ]; then
  DONTCOPY=0
fi

if [ $DONTCOPY -eq 0  ]; then
  echo "The built distribution will be copied to the mpgedit website"
  echo "Enter ^C to interrupt if this is not what you want to do"
  for i in 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1; do
    echo -n "$i "
    sleep 1
  done
fi

#
# Run the make_dist_sdk_script.sh to build the mpgedit SDK
#
echo cd $dir
echo src/mpgedit/make_dist_sdk.sh
(cd $dir; src/mpgedit/make_dist_sdk.sh)

##
## Build mpgedit SDK
##
#(cd $dir/linux/mpgedit;     ./make_sdk.pl $RELEASE linux)
#(cd $dir/win32/mpgedit;     ./make_sdk.pl $RELEASE win32)


#
# Install mpgedit SDK
#
#(cd $dir/linux/mpgedit; $scriptdir/make_sdk.pl $RELEASE linux)
#(cd $dir/win32/mpgedit; $scriptdir/make_sdk.pl $RELEASE win32)

#
# Build the distribution packages areas
#
(cd $dir/linux/mpgedit;     ./dist_linux.sh     $RELEASE)
(cd $dir/linux_rh7/mpgedit; ./dist_linux_rh7.sh $RELEASE)
(cd $dir/linux_fedora8/mpgedit; ./dist_linux_fedora8.sh $RELEASE)
(cd $dir/macosx/mpgedit;    ./dist_macosx.sh    $RELEASE)
(cd $dir/win32/mpgedit;     ./dist_win32.sh     $RELEASE)
./dist_src.sh "$RELEASE" "$TAG"

#
# Tar/zip them up
#
(
if [ ! -s $dir/mpgedit_${RELEASE}_linux.tgz ]; then
  cd $dir
  tar zcf mpgedit_${RELEASE}_linux.tgz mpgedit_${RELEASE}_linux
  src/mpgedit/make_shtgzexe.sh mpgedit_${RELEASE}_linux.tgz
fi
)
(
if [ ! -s $dir/mpgedit_${RELEASE}_linux_rh7.tgz ]; then
  cd $dir
  tar zcf mpgedit_${RELEASE}_linux_rh7.tgz mpgedit_${RELEASE}_linux_rh7
  src/mpgedit/make_shtgzexe.sh mpgedit_${RELEASE}_linux_rh7.tgz
fi
)
(
if [ ! -s $dir/mpgedit_${RELEASE}_linux_fedora8.tgz ]; then
  cd $dir
  tar zcf mpgedit_${RELEASE}_linux_fedora8.tgz mpgedit_${RELEASE}_linux_fedora8
  src/mpgedit/make_shtgzexe.sh mpgedit_${RELEASE}_linux_fedora8.tgz
fi
)
(
if [ ! -s $dir/mpgedit_${RELEASE}_macosx.tgz ]; then
  cd $dir
  tar zcf mpgedit_${RELEASE}_macosx.tgz mpgedit_${RELEASE}_macosx
  src/mpgedit/make_shtgzexe.sh mpgedit_${RELEASE}_macosx.tgz
fi
)
#
# This is now done by dist_src.sh
#
#(
#if [ ! -s $dir/mpgedit_${RELEASE}_src.tgz ]; then
#  cd $dir
#  tar zcf mpgedit_${RELEASE}_src.tgz mpgedit_${RELEASE}_src
#  src/mpgedit/make_shtgzexe.sh mpgedit_${RELEASE}_src.tgz
#fi
#)
(
if [ ! -s $dir/mpgedit_${RELEASE}_win32.zip ]; then
  cd $dir; zip -r mpgedit_${RELEASE}_win32.zip mpgedit_${RELEASE}_win32
fi
)
(
if [ ! -s $dir/setup_mpgedit_${RELEASE}.exe ]; then
  cd $dir; cp win32/mpgedit/Output/setup.exe setup_mpgedit_${RELEASE}.exe 
fi
)
(
if [ ! -s $dir/setup_xmpgedit_${RELEASE}.exe ]; then
  cd $dir; cp win32/mpgedit/gui/Output/setup.exe setup_xmpgedit_${RELEASE}.exe 
fi
)

#
# Copy these distribution files to the local copy of mpgedit web site
# 

if [ $DONTCOPY -eq 1 ]; then
  echo "Copying packages to mpgedit web site suppressed"
  exit 0
fi

(

  cat NEWS | sed -e '1i\
<HTML><pre>'     -e '$a\
</pre></HTML>' > /tmp/NEWS_0_7.html

  cd $dir
  cat << NNNN > cpit.sh
su number6 -c 'cp mpgedit_${RELEASE}_linux_tgz.zip /home/number6/html/mpgedit/download'
su number6 -c 'cp mpgedit_${RELEASE}_linux_rh7_tgz.zip /home/number6/html/mpgedit/download'
su number6 -c 'cp mpgedit_${RELEASE}_linux_fedora8_tgz.zip /home/number6/html/mpgedit/download'
su number6 -c 'cp mpgedit_${RELEASE}_src_tgz.zip /home/number6/html/mpgedit/download'
su number6 -c 'cp mpgedit_${RELEASE}_win32.zip /home/number6/html/mpgedit/download'
su number6 -c 'cp win32/mpgedit/gui/Output/setup.exe /home/number6/html/mpgedit/download/setup_xmpgedit_${RELEASE}.exe'
su number6 -c 'cp win32/mpgedit/Output/setup.exe /home/number6/html/mpgedit/download/setup_mpgedit_${RELEASE}.exe'
su number6 -c 'cp linux/mpgedit/mpgedit_sdk_linux_${RELEASE}_tgz.zip /home/number6/html/mpgedit/download'
su number6 -c 'cp win32/mpgedit/mpgedit_sdk.zip /home/number6/html/mpgedit/download'
su number6 -c 'cp /tmp/NEWS_0_7.html /home/number6/html/mpgedit'
NNNN
  sh -v cpit.sh
  rm -f /tmp/NEWS_0_7.html

  # Update downloads.html page to contain new alpha version.
  # This has become complicated enough that a real sed script
  # is needed to do this work.
  #
  cat<<NNNN>/tmp/sedscript$$
s|>Last modified: .*<|>Last modified: `date +%x`<|
s|[0-9]-[0-9]*beta[0-9][0-9]*|$ALPHA|
/[0-9]-[0-9]*beta-[0-9][0-9]*/{
  s|[0-9]-[0-9]*beta-[0-9][0-9]*|$ALPHA|
  s|\(beta\)\([0-9][0-9]*\)|\1-\2|
}
/setup_xmpgedit_.*.exe/{
  s|setup_xmpgedit_.*\.exe|setup_xmpgedit_${RELEASE}.exe|
}
NNNN

  # Run the generated sed script above
  #
  cat /home/number6/html/mpgedit/downloads.html | \
    sed -f /tmp/sedscript$$ > /tmp/downloads.html.edited

  if [ -s /tmp/downloads.html.edited ]; then
    cp /tmp/downloads.html.edited /home/number6/html/mpgedit/downloads.html
    cp /tmp/downloads.html.edited /home/number6/html/mpgedit
  fi

  # Update home.html page to contain new alpha version
  #
  cat /home/number6/html/mpgedit/home.html | \
    sed -e "s|>Last modified: .*<|>Last modified: `date +%x`<|" \
        -e "s|beta[0-9][0-9]*|$ALPHA|" > \
    /tmp/home.html.edited
  if [ -s /tmp/home.html.edited ]; then
    cp /tmp/home.html.edited /home/number6/html/mpgedit/home.html
    cp /tmp/home.html.edited /home/number6/html/mpgedit
  fi
)

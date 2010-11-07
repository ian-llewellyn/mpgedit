#!/bin/sh
#
# Run this from the top level project directory with a
# command line similar to:
#
# cd /home/number6/wrk/audio/mpgedit/dev/mpgedit_0-73dev1/
# src/mpgedit/make_dist_sdk.sh
#

if [ -n "$1" ]; then
  products="$1"
else
  products="MPGEDIT_SDK_PRODUCT"
fi

. `dirname $0`/dist_parse_version.sh $0
echo debug 1 RELEASE=$RELEASE

for i in $products; do
  echo "debug products='$i'"
  grep " $i " $VERSIONFILE
  if [ -z "`grep " $i " $VERSIONFILE`" -o \
       -z "`echo $i | grep '_PRODUCT$'`" ]; then
    echo "ERROR: product '$i' not found"
    exit 1
  fi

  PRODUCTX=`grep " $i " $VERSIONFILE  |      \
           sed -e "s/#define  *$i  *//" \
               -e 's/[ "-]//g' \
               -e 's/\./-/'`
  product=`echo $i | sed -e 's/_PRODUCT$/_VERSION/'`

  RELEASEX=`grep " $product " $VERSIONFILE  |      \
           sed -e "s/#define  *$product  *//" \
               -e 's/[ "-]//g' \
               -e 's/\./-/'`
  if [ "$i" = "MPGEDIT_SDK_PRODUCT" ]; then
    SDKRELEASE="$RELEASEX"
  fi
  RELEASE="${RELEASE}_${PRODUCTX}-${RELEASEX}"
done

#ALPHA=`echo $RELEASE | sed -e 's/0-7//'`
HTMLTOP=/home/number6/html/mpgedit

echo debug RELEASE=$RELEASE
echo debug SDKRELEASE=$SDKRELEASE

if [ -z "$SDKRELEASE" ]; then
  echo "ERROR: SDKRELEASE version is not specified"
  exit 1
fi

dir="mpgedit_$RELEASE"
echo dir=$dir

uid=`id | awk '{print $1}' | sed -e 's/uid=//' -e 's/(.*//'`
if [ $uid -ne 0 ]; then
  echo "you must be root to continue executing from this point"
  exit 1
fi

#
# Build mpgedit SDK
#
if [ ! -d linux -o ! -d win32 ]; then
  echo "Error: Can't find Linux and Win32 build areas"
  echo "       Are you in the correct top level directory?"
  exit 1
fi

mkdir -p $dir
(cd linux/mpgedit;     ./make_sdk.pl $RELEASE linux)
(cd win32/mpgedit;     ./make_sdk.pl $RELEASE win32)

#zzz
# exit 1 ### testing!!!

# Don't forget about the source code...
(
if [ ! -s $dir/mpgedit_${RELEASE}_src.tar.gz ]; then
  cd $dir
#  cvs -d /usr/local/cvsroot export -d mpgedit_${RELEASE}_src -r HEAD mpgedit
  cvs export -d mpgedit_${RELEASE}_src -r HEAD mpgedit
  tar zcf mpgedit_${RELEASE}_src.tar.gz mpgedit_${RELEASE}_src 
fi
)

#
# Link the built files into the top-level directory
#
LINUX_SDK_NAME=mpgedit_sdk_linux_${RELEASE}.tgz
WIN32_SDK_ZIP=mpgedit_sdk_win32_${RELEASE}.zip
WIN32_SDK_EXE=mpgedit_sdk_win32_${RELEASE}.exe
ln -s ../linux/mpgedit/$LINUX_SDK_NAME $dir
ln -s ../win32/mpgedit/$WIN32_SDK_ZIP $dir
ln -s ../win32/mpgedit/Output/mpgedit_sdk.exe $dir/$WIN32_SDK_EXE

exit 0
### everything below this is skipped!!!
#
#
# Copy the SDK distributions into the web site download directories
#

(cd $dir/src/mpgedit
 cat NEWS | sed -e '1i\
<HTML><pre>'     -e '$a\
</pre></HTML>' > /tmp/NEWS_0_7.html

 cat README_SDK.linux | sed -e '1i\
<HTML><pre>'     -e '$a\
</pre></HTML>' > /tmp/README_SDK_linux.html

 cat README_SDK.win32 | sed -e '1i\
<HTML><pre>'     -e '$a\
</pre></HTML>' > /tmp/README_SDK_win32.html
)

cat <<NNNN>/tmp/cpit
su number6 -c 'cp /tmp/NEWS_0_7.html $HTMLTOP'
su number6 -c 'cp $dir/linux/mpgedit/mpgedit_sdk_linux_${RELEASE}.tgz $HTMLTOP/download/mpgedit_sdk_linux_${SDKRELEASE}.tgz'
su number6 -c 'cp $dir/win32/mpgedit/Output/mpgedit_sdk.exe $HTMLTOP/download/mpgedit_sdk_win32_${SDKRELEASE}.exe'
su number6 -c 'cp $dir/mpgedit_${RELEASE}_src.tar.gz $HTMLTOP/download'
su number6 -c 'cp /tmp/README_SDK_linux.html $HTMLTOP/download'
su number6 -c 'cp /tmp/README_SDK_win32.html $HTMLTOP/download'
su number6 -c 'cp $dir/src/mpgedit/contrib/python/py_mpgedit/py_mpgedit_license.htm $HTMLTOP/sdk'
su number6 -c 'cp $dir/src/mpgedit/contrib/python/py_mpgedit/py_mpgedit.htm $HTMLTOP/sdk'
su number6 -c 'cp $dir/src/mpgedit/contrib/python/py_mpgedit/python_297x100.png $HTMLTOP/sdk'
NNNN

#sh -v /tmp/cpit

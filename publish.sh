#!/bin/sh
#
# This script copies all built files into the HTML downloads area.
#
. linux/mpgedit/dist_parse_version.sh linux/mpgedit/dist_parse_version.sh
if [ -z "$CKPT" ]; then
  echo "ERROR: invalid checkpoint '$CKPT'"
  exit 1
fi

HTMLDIR=$HOME/html/mpgedit/download/${MAJOR}_${MINOR}
if [ ! -d $HTMLDIR ]; then
  echo "ERROR: distribution directory '$HTMLDIR' does not exist"
  exit 1
fi

version=$CKPT
sts=""
#
# Verify all files in distribution are present
#
[ -s mpgedit-0.75.${version}-1.src.rpm ] || sts="$sts mpgedit-0.75.${version}-1.src.rpm"
[ -s mpgedit-0.75.${version}.6-1.i386.rpm ] || sts="$sts mpgedit-0.75.${version}.6-1.i386.rpm"
[ -s mpgedit-0.75.${version}.7-1.i386.rpm ] || sts="$sts mpgedit-0.75.${version}.7-1.i386.rpm"
[ -s mpgedit-0.75.${version}.f8-1.i386.rpm ] || sts="$sts mpgedit-0.75.${version}.f8-1.i386.rpm"
[ -s mpgedit-man-0.75.${version}-1.src.rpm ] || sts="$sts mpgedit-man-0.75.${version}-1.src.rpm"
[ -s mpgedit-man-0.75.${version}.6-1.noarch.rpm ] || sts="$sts mpgedit-man-0.75.${version}.6-1.noarch.rpm"
[ -s mpgedit-man-0.75.${version}.7-1.noarch.rpm ] || sts="$sts mpgedit-man-0.75.${version}.7-1.noarch.rpm"
[ -s mpgedit-man-0.75.${version}.f8-1.noarch.rpm ] || sts="$sts mpgedit-man-0.75.${version}.f8-1.noarch.rpm"
[ -s mpgedit_0-75dev${version}_linux.tgz ] || sts="$sts mpgedit_0-75dev${version}_linux.tgz"
[ -s mpgedit_0-75dev${version}_linux_fedora8.tgz ] || sts="$sts mpgedit_0-75dev${version}_linux_fedora8.tgz"
[ -s mpgedit_0-75dev${version}_linux_rh7.tgz ] || sts="$sts mpgedit_0-75dev${version}_linux_rh7.tgz"
[ -s mpgedit_0-75dev${version}_macosx.tgz ] || sts="$sts mpgedit_0-75dev${version}_macosx.tgz"
[ -s mpgedit_0-75dev${version}_src.tgz ] || sts="$sts mpgedit_0-75dev${version}_src.tgz"
[ -s mpgedit_0-75dev${version}_win32.zip ] || sts="$sts mpgedit_0-75dev${version}_win32.zip"
[ -s mpgedit_pkg_0-75dev${version}.dmg ] || sts="$sts mpgedit_pkg_0-75dev${version}.dmg"
[ -s setup_mpgedit_0-75dev${version}.exe ] || sts="$sts setup_mpgedit_0-75dev${version}.exe"
[ -s setup_xmpgedit_0-75dev${version}.exe ] || sts="$sts setup_xmpgedit_0-75dev${version}.exe"
[ -s xmpgedit-0.75.${version}-1.src.rpm ] || sts="$sts xmpgedit-0.75.${version}-1.src.rpm"
[ -s xmpgedit-0.75.${version}.6-1.i386.rpm ] || sts="$sts xmpgedit-0.75.${version}.6-1.i386.rpm"
[ -s xmpgedit-0.75.${version}.7-1.i386.rpm ] || sts="$sts xmpgedit-0.75.${version}.7-1.i386.rpm"
[ -s xmpgedit-0.75.${version}.f8-1.i386.rpm ] || sts="$sts xmpgedit-0.75.${version}.f8-1.i386.rpm"
[ -s xmpgedit_0-75dev${version}.dmg ] || sts="$sts xmpgedit_0-75dev${version}.dmg"
[ -s xmpgedit_gtk_pkg_0-75dev${version}.dmg ] || sts="$sts xmpgedit_gtk_pkg_0-75dev${version}.dmg"

if [ -n "$sts"  ]; then
  echo "ERROR: File '$sts' is missing"
  exit 1
fi


#
# Copy all files into distribution directory
#
cp -v "mpgedit-0.75.${version}-1.src.rpm" $HTMLDIR
cp -v "mpgedit-0.75.${version}.6-1.i386.rpm" $HTMLDIR
cp -v "mpgedit-0.75.${version}.7-1.i386.rpm" $HTMLDIR
cp -v "mpgedit-0.75.${version}.f8-1.i386.rpm" $HTMLDIR
cp -v "mpgedit-man-0.75.${version}-1.src.rpm" $HTMLDIR
cp -v "mpgedit-man-0.75.${version}.6-1.noarch.rpm" $HTMLDIR
cp -v "mpgedit-man-0.75.${version}.7-1.noarch.rpm" $HTMLDIR
cp -v "mpgedit-man-0.75.${version}.f8-1.noarch.rpm" $HTMLDIR
cp -v "mpgedit_0-75dev${version}_linux.tgz" $HTMLDIR
cp -v "mpgedit_0-75dev${version}_linux_fedora8.tgz" $HTMLDIR
cp -v "mpgedit_0-75dev${version}_linux_rh7.tgz" $HTMLDIR
cp -v "mpgedit_0-75dev${version}_macosx.tgz" $HTMLDIR
cp -v "mpgedit_0-75dev${version}_src.tgz" $HTMLDIR
cp -v "mpgedit_0-75dev${version}_win32.zip" $HTMLDIR
cp -v "mpgedit_pkg_0-75dev${version}.dmg" $HTMLDIR
cp -v "setup_mpgedit_0-75dev${version}.exe" $HTMLDIR
cp -v "setup_xmpgedit_0-75dev${version}.exe" $HTMLDIR
cp -v "xmpgedit-0.75.${version}-1.src.rpm" $HTMLDIR
cp -v "xmpgedit-0.75.${version}.6-1.i386.rpm" $HTMLDIR
cp -v "xmpgedit-0.75.${version}.7-1.i386.rpm" $HTMLDIR
cp -v "xmpgedit-0.75.${version}.f8-1.i386.rpm" $HTMLDIR
cp -v "xmpgedit_0-75dev${version}.dmg" $HTMLDIR
cp -v "xmpgedit_gtk_pkg_0-75dev${version}.dmg" $HTMLDIR

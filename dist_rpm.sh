#!/bin/sh -x
#
# Script to build mpgedit RPMS from a source code bundle. 
# How to use
# 1) First, run the production build script make_dist.sh to
#    successful completion
# 2) Copy or symlink the source package into the "src" directory
#    of the production build tree.  The source package is named something
#    like mpgedit_0-71dev_src.exe.
# 3) Become root.
# 4) Run ./dist_rpm.sh rev_num
#    Note: Unless you modify all of the *.spec files rev numbers to match
#    the rev you are building, just use the version that matches the spec
#    rev number you have.
# 5) Sit back and watch the fun. 
# 6) Do this on jake for the RedHat 6 RPM build first.
# 7) Repeat this on batman for the RedHat 7 RPM build.
#
#define MPGEDIT_VERSION   "0.72 beta2"
#                     MAJ--^ ^  ^   ^--CKPT
#                    MIN-----+  |
#                               +-----RELTYPE

# dist_parse_version.sh sets "RELEASE" from version.h
. `dirname $0`/dist_parse_version.sh $0

RPMBASE="/usr/src/redhat"
DIR=`dirname $0`

SRC="mpgedit_${RELEASE}_src"
#SRC_EXT=".exe"
SRC_EXT=".tgz"
MAJ="${MAJOR}.${MINOR}"
REV="1"
RPM="rpmbuild"
host=`hostname | sed 's/\..*//'`
if [ "$host" = jake ]; then
  RPM="rpm"
  TARGET="6"
elif [ "$host" = quatro ]; then
  TARGET="f8"
else
  TARGET="7"
fi


# Checkpoint release number is mandatory
# This is now obtained from the version.h file.
#
#if [ -z "$1" ]; then
#  echo "ERROR: Must specify checkpoint release version number"
#  exit 1
#fi

RELEASE="$CKPT"
shift

#echo "*** debug SRC=$SRC ***"
#echo "*** debug RELEASE=$RELEASE ***"

# Revision number is optional.  Default value is 1.
#
if [ -n "$1" ]; then
  REV="$1"
  shift
fi

uid=`id -u`
if [ $uid -ne 0 ]; then
  echo "you must be root to to run dist_rpm.sh"
  exit 1
fi

# mpgedit-0.71.49-1.i386.spec  mpgedit-man-0.71.49-noarch.spec \
# xmpgedit-0.71.49-1.i386.spec
MPGEDITMAN="mpgedit-man-$MAJ.${CKPT}-noarch.spec"
MPGEDIT="mpgedit-$MAJ.${CKPT}-${REV}.i386.spec"
XMPGEDIT="xmpgedit-$MAJ.${CKPT}-${REV}.i386.spec"

#mpgedit-man-0.72.2-noarch.spec

#echo MPGEDITMAN=$MPGEDITMAN
#echo MPGEDIT=$MPGEDIT
#echo XMPGEDIT=$XMPGEDIT
#exit 1

#mpgedit-man-0.71-49.i386.rpm
MPGEDITMANRPM="mpgedit-man-$MAJ-${RELEASE}.i386.rpm"
MPGEDITRPM="mpgedit-$MAJ-${RELEASE}.i386.rpm"
XMPGEDITRPM="xmpgedit-$MAJ-${RELEASE}.i386.rpm"

# Only need mpgedit source rpm, since it is the parent
# of all of the other packages.
#
MPGEDITSRCRPM="mpgedit-$MAJ-${RELEASE}.src.rpm"
XMPGEDITSRCRPM="xmpgedit-$MAJ-${RELEASE}.src.rpm"
MPGEDITMANSRCRPM="mpgedit-man-$MAJ-${RELEASE}.src.rpm"

#
# Cleanup previous build.
#
if [ -f "$RPMBASE/SRPMS/$MPGEDITSRCRPM" ]; then
  rm -f "$RPMBASE/SRPMS/$MPGEDITSRCRPM"
fi

if [ -f "$RPMBASE/SRPMS/$MPGEDITMANSRCRPM" ]; then
  rm -f "$RPMBASE/SRPMS/$MPGEDITMANSRCRPM"
fi

if [ -f "$RPMBASE/SRPMS/$XMPGEDITSRCRPM" ]; then
  rm -f "$RPMBASE/SRPMS/$XMPGEDITSRCRPM"
fi

if [ -f "$RPMBASE/RPMS/i386/$MPGEDITMANRPM" ]; then
  rm -f "$RPMBASE/RPMS/i386/$MPGEDITMANRPM"
fi

if [ -f "$RPMBASE/RPMS/i386/$MPGEDITRPM" ]; then
  rm -f  "$RPMBASE/RPMS/i386/$MPGEDITRPM"
fi

if [ -f "$RPMBASE/RPMS/i386/$XMPGEDITRPM" ]; then
  rm -f "$RPMBASE/RPMS/i386/$XMPGEDITRPM"
fi

#Example rpms in build directory...
#
#/usr/src/redhat/RPMS/i386/mpgedit-0.72-0.i386.rpm
#/usr/src/redhat/RPMS/i386/mpgedit-man-0.72-0.i386.rpm
#/usr/src/redhat/RPMS/i386/xmpgedit-0.72-0.i386.rpm
#/usr/src/redhat/SRPMS/mpgedit-0.72-0.src.rpm
#/usr/src/redhat/SRPMS/mpgedit-man-0.72-0.src.rpm
#/usr/src/redhat/SRPMS/xmpgedit-0.72-0.src.rpm

MPGEDITMANTGTRPM="mpgedit-man-$MAJ.${RELEASE}.${TARGET}-${REV}.noarch.rpm"
MPGEDITTGTRPM="mpgedit-$MAJ.${RELEASE}.${TARGET}-${REV}.i386.rpm"
XMPGEDITTGTRPM="xmpgedit-$MAJ.${RELEASE}.${TARGET}-${REV}.i386.rpm"

MPGEDITSRCTGTRPM="mpgedit-$MAJ.${RELEASE}-${REV}.src.rpm"
XMPGEDITSRCTGTRPM="xmpgedit-$MAJ.${RELEASE}-${REV}.src.rpm"
MPGEDITMANSRCTGTRPM="mpgedit-man-$MAJ.${RELEASE}-${REV}.src.rpm"


#
# Copy the template .spec files to the release version names
#
if [ ! -f ${MPGEDITMAN} ]; then
  cp "mpgedit-man-template-noarch.spec" ${MPGEDITMAN}
fi

if [ ! -f ${MPGEDIT} ]; then
  cp "mpgedit-template.i386.spec" ${MPGEDIT}
fi

if [ ! -f ${XMPGEDIT} ]; then
  cp "xmpgedit-template.i386.spec" ${XMPGEDIT}
fi

#
# Fix up the RELEASE tags in the RPM spec files
# 
if [ ! -f ${MPGEDITMAN}.orig ]; then
  mv ${MPGEDITMAN} ${MPGEDITMAN}.orig
  cat ${MPGEDITMAN}.orig | $DIR/mk_install_version.sh > ${MPGEDITMAN}
  $DIR/mk_rpm_changelog.pl NEWS >> ${MPGEDITMAN}
fi

if [ ! -f ${MPGEDIT}.orig ]; then
  mv ${MPGEDIT} ${MPGEDIT}.orig
  cat ${MPGEDIT}.orig | $DIR/mk_install_version.sh > ${MPGEDIT}
  $DIR/mk_rpm_changelog.pl NEWS >> ${MPGEDIT}
fi

if [ ! -f ${XMPGEDIT}.orig ]; then
  mv ${XMPGEDIT} ${XMPGEDIT}.orig
  cat ${XMPGEDIT}.orig | $DIR/mk_install_version.sh > ${XMPGEDIT}
  $DIR/mk_rpm_changelog.pl NEWS >> ${XMPGEDIT}
fi


if [ -f "$RPMBASE/BUILD/${SRC}.tar" ]; then
    rm -f "$RPMBASE/BUILD/${SRC}.tar" 
fi

if [ -d "$RPMBASE/BUILD/$SRC" ]; then
   rm -rf "$RPMBASE/BUILD/$SRC" 
fi

if [ ! -f "${SRC}${SRC_EXT}" ]; then
  echo "ERROR: Unable to find mpgedit source package"
  exit 1
fi
#cp "${SRC}${SRC_EXT}" "$RPMBASE/SOURCES"
cp "${SRC}${SRC_EXT}" "$RPMBASE/SOURCES"
if [ ! -f $MPGEDIT ]; then
  if [ -f "$DIR/$MPGEDIT" ]; then
    (
    cd $DIR
    $RPM -ba $MPGEDIT
    $RPM -ba $MPGEDITMAN
    $RPM -ba $XMPGEDIT
    )
  else
    echo "ERROR: Unable to find RPM spec files"
    exit 1
  fi
else
  $RPM -ba $MPGEDIT
  $RPM -ba $MPGEDITMAN
  $RPM -ba $XMPGEDIT
fi

# Copy down the built RPMS into the local build area.
#
if [ -f "$RPMBASE/RPMS/i386/$MPGEDITMANRPM" ]; then
  if [ ! -f "$MPGEDITMANTGTRPM" ]; then
    cp "$RPMBASE/RPMS/i386/$MPGEDITMANRPM" "$MPGEDITMANTGTRPM"
    [ -s "../../$MPGEDITMANTGTRPM" ] || ln -s  `pwd`/"$MPGEDITMANTGTRPM" ../..
  fi
else
  echo "ERROR: mpgedit man page RPM does not exist"
fi

if [ -f "$RPMBASE/RPMS/i386/$MPGEDITRPM" ]; then
  if [ ! -f "$MPGEDITTGTRPM" ]; then
    cp "$RPMBASE/RPMS/i386/$MPGEDITRPM" "$MPGEDITTGTRPM"
    [ -s "../../$MPGEDITTGTRPM" ] ||  ln -s `pwd`/"$MPGEDITTGTRPM" ../..
  fi
else
  echo "ERROR: mpgedit RPM does not exist"
fi

if [ -f "$RPMBASE/RPMS/i386/$XMPGEDITRPM" ]; then
  if [ ! -f "$XMPGEDITTGTRPM" ]; then
    cp "$RPMBASE/RPMS/i386/$XMPGEDITRPM" "$XMPGEDITTGTRPM"
    [ -s "../../$XMPGEDITTGTRPM" ] || ln -s `pwd`/"$XMPGEDITTGTRPM" ../..
  fi
else
  echo "ERROR: xmpgedit RPM does not exist"
fi

# Copy down the mpgedit SRPM into the local build area.
#
if [ -f "$RPMBASE/SRPMS/$MPGEDITSRCRPM" ]; then
  if [ ! -f "$MPGEDITSRCTGTRPM" ]; then
    cp "$RPMBASE/SRPMS/$MPGEDITSRCRPM" "$MPGEDITSRCTGTRPM"
    [ -s "../../$MPGEDITSRCTGTRPM" ] || ln -s `pwd`/"$MPGEDITSRCTGTRPM" ../..
  fi
else
  echo "ERROR: mpgedit SRPM does not exist"
fi

# Copy down the xmpgedit SRPM into the local build area.
#
if [ -f "$RPMBASE/SRPMS/$XMPGEDITSRCRPM" ]; then
  if [ ! -f "$XMPGEDITSRCTGTRPM" ]; then
    cp "$RPMBASE/SRPMS/$XMPGEDITSRCRPM" "$XMPGEDITSRCTGTRPM"
    [ -s "../../$XMPGEDITSRCTGTRPM" ] || ln -s `pwd`/"$XMPGEDITSRCTGTRPM" ../..
  fi
else
  echo "ERROR: xmpgedit SRPM does not exist"
fi

# Copy down the mpgedit man SRPM into the local build area.
#
if [ -f "$RPMBASE/SRPMS/$MPGEDITMANSRCRPM" ]; then
  if [ ! -f "$MPGEDITMANSRCTGTRPM" ]; then
    cp "$RPMBASE/SRPMS/$MPGEDITMANSRCRPM" "$MPGEDITMANSRCTGTRPM"
    [ -s "../../$MPGEDITMANSRCTGTRPM" ] || ln -s `pwd`/"$MPGEDITMANSRCTGTRPM" ../..
  fi
else
  echo "ERROR: mpgedit SRPM does not exist"
fi

exit 0

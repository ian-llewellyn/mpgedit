#!/bin/sh
#define MPGEDIT_VERSION   "0.72 beta2"
#                     MAJ--^ ^  ^   ^--CKPT
#                    MIN-----+  |
#                               +-----RELTYPE
#
#
#define XMPGEDIT_VERSION  "0.10 beta2"
#                          ^ ^  ^   ^
#                          | |  |   |
#                    XMAJ--+ |  |   +---CKPT
#                 XMIN-------+  +-----RELTYPE
#
#MAJOR=0
#MINOR=72
#RELTYPE=beta
#CKPT=2
#
#VERSIONFILE=/home/number6/wrk/audio/mpgedit/production/mpgedit_0-72beta2.work/src/mpgedit/version.h
#TAG=mpgedit_0-72_sup_branchroot
#ALPHA=2beta2
#RELEASE=0-72beta2
#define MPGEDIT_SDK_VERSION "0.3 beta"
#


parse_version()
{
export PATH=$PATH:/opt/bin
scriptdir=`dirname $0`
pwd=`pwd`
cd $scriptdir
scriptdir=`pwd`
cd $pwd
if [ -d $scriptdir ]; then
  VERSIONFILE="$scriptdir/version.h"
else
  scriptdir="/home/number6/wrk/audio/mpgedit/dev/mpgedit"
  VERSIONFILE="$scriptdir/version.h"
fi

#echo VERSIONFILE=$VERSIONFILE
#\([0-9]\)\.\([0-9][0-9]*\) \([a-z][a-z]*\)\([0-9]*\)
MAJOR=`grep ' MPGEDIT_VERSION ' $VERSIONFILE  | \
         sed -e 's/.*"\(.*\)".*/\1/' \
             -e 's/\([0-9]\)\.\([0-9][0-9]*\) \([a-z][a-z]*\)\([0-9]*\)/\1/'`
MINOR=`grep ' MPGEDIT_VERSION ' $VERSIONFILE  | \
         sed -e 's/.*"\(.*\)".*/\1/' \
             -e 's/\([0-9]\)\.\([0-9][0-9]*\) \([a-z][a-z]*\)\([0-9]*\)/\2/'`
RELTYPE=`grep ' MPGEDIT_VERSION ' $VERSIONFILE  | \
         sed -e 's/.*"\(.*\)".*/\1/' \
             -e 's/\([0-9]\)\.\([0-9][0-9]*\) \([a-z][a-z]*\)\([0-9]*\)/\3/'`
CKPT=`grep ' MPGEDIT_VERSION ' $VERSIONFILE  | \
         sed -e 's/.*"\(.*\)".*/\1/' \
             -e 's/\([0-9]\)\.\([0-9][0-9]*\) \([a-z][a-z]*\)\([0-9]*\)/\4/'`

XMAJOR=`grep ' XMPGEDIT_VERSION ' $VERSIONFILE  | \
         sed -e 's/.*"\(.*\)".*/\1/' \
             -e 's/\([0-9]\)\.\([0-9][0-9]*\) \([a-z][a-z]*\)\([0-9]*\)/\1/'`
XMINOR=`grep ' XMPGEDIT_VERSION ' $VERSIONFILE  | \
         sed -e 's/.*"\(.*\)".*/\1/' \
             -e 's/\([0-9]\)\.\([0-9][0-9]*\) \([a-z][a-z]*\)\([0-9]*\)/\2/'`
XRELTYPE=`grep ' XMPGEDIT_VERSION ' $VERSIONFILE  | \
         sed -e 's/.*"\(.*\)".*/\1/' \
             -e 's/\([0-9]\)\.\([0-9][0-9]*\) \([a-z][a-z]*\)\([0-9]*\)/\3/'`
XCKPT=`grep ' XMPGEDIT_VERSION ' $VERSIONFILE  | \
         sed -e 's/.*"\(.*\)".*/\1/' \
             -e 's/\([0-9]\)\.\([0-9][0-9]*\) \([a-z][a-z]*\)\([0-9]*\)/\4/'`
SDKVER=`grep ' MPGEDIT_SDK_VERSION ' $VERSIONFILE  | \
         sed -e 's/.*"\(.*\)".*/\1/' \
             -e 's/  *//'`

SDKMAJOR=`grep ' MPGEDIT_SDK_VERSION ' $VERSIONFILE  | \
           sed -e 's/.*"\(.*\)".*/\1/' \
               -e 's/\([0-9]\)\.\([0-9][0-9]*\) \([a-z][a-z]*\)\([0-9]*\)/\1/'`

SDKMINOR=`grep ' MPGEDIT_SDK_VERSION ' $VERSIONFILE  | \
           sed -e 's/.*"\(.*\)".*/\1/' \
               -e 's/\([0-9]\)\.\([0-9][0-9]*\) \([a-z][a-z]*\)\([0-9]*\)/\2/'`

SDKRELTYPE=`grep ' MPGEDIT_SDK_VERSION ' $VERSIONFILE  | \
           sed -e 's/.*"\(.*\)".*/\1/' \
               -e 's/\([0-9]\)\.\([0-9][0-9]*\) \([a-z][a-z]*\)\([0-9]*\)/\3/'`

SDKRELTYPEUC=`grep ' MPGEDIT_SDK_VERSION ' $VERSIONFILE  | \
           sed -e 's/.*"\(.*\)".*/\1/' \
               -e 's/\([0-9]\)\.\([0-9][0-9]*\) \([a-z][a-z]*\)\([0-9]*\)/\3/' | \
                tr '[a-z]' '[A-Z]'`

#echo debug MAJOR=$MAJOR
#echo debug MINOR=$MINOR
#echo debug RELTYPE=$RELTYPE
#echo debug CKPT=$CKPT
#echo debug XMAJOR=$XMAJOR
#echo debug XMINOR=$XMINOR
#echo debug XRELTYPE=$XRELTYPE
#echo debug XCKPT=$XCKPT

RELEASE="${MAJOR}-${MINOR}${RELTYPE}${CKPT}"
XRELEASE="${XMAJOR}-${XMINOR}${XRELTYPE}${XCKPT}"

TAG=`grep ' MPGEDIT_CVSTAG ' $VERSIONFILE | \
     sed -e 's/#define  *MPGEDIT_CVSTAG  *//' \
         -e 's/"//g'`
ALPHA=`echo $RELEASE | sed -e 's/0-7//'`

if [ -z "$RELEASE" ]; then
  RELEASE="0-7beta"
fi

#echo VERSIONFILE=$VERSIONFILE
#echo TAG=$TAG
#echo ALPHA=$ALPHA
#echo RELEASE=$RELEASE
}

parse_version $0

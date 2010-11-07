#!/bin/sh
#
# Script to fixup a file with version/release tags.  These version
# values are obtained from version.h, and parsed by dist_parse_version.sh
#
do_stdin=0
if [ -z "$1" ]; then
  do_stdin=1
else
  infile=$1
fi
. `dirname $0`/dist_parse_version.sh $0
MONTH=`date +%B`
DAY=`date +%d`
YEAR=`date +%Y`

cat <<NNNN> mkinstallversion_sedit
s/__RELEASE__/$RELEASE/g
s/__MAJOR__/$MAJOR/g
s/__MINOR__/$MINOR/g
s/__RELTYPE__/$RELTYPE/g
s/__CKPOINT__/$CKPT/g
s/__XRELEASE__/$XRELEASE/g
s/__XMAJOR__/$XMAJOR/g
s/__XMINOR__/$XMINOR/g
s/__XRELTYPE__/$XRELTYPE/g
s/__XCKPOINT__/$XCKPT/g
s/__MONTH__/$MONTH/g
s/__DAY__/$DAY/g
s/__YEAR__/$YEAR/g
s/__SDKVER__/$SDKVER/g
s/__SDKMAJOR__/$SDKMAJOR/g
s/__SDKMINOR__/$SDKMINOR/g
s/__SDKRELTYPE__/$SDKRELTYPE/g
s/__SDKRELTYPEUC__/$SDKRELTYPEUC/g
NNNN

if [ $do_stdin -eq 1 ]; then
  sed -f mkinstallversion_sedit
else
  sed -f mkinstallversion_sedit $infile
fi
rm mkinstallversion_sedit

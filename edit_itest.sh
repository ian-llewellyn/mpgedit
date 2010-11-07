#!/bin/sh
#
# Script to test mpgedit -I editing.  There was a bug that caused random
# repeats at the start of some edits due to a missing initialization of
# an mpegiobuf structure.  This script exercises that editing.
#
if [ -n "$1" ]; then
  inmp3="$1"
else
  inmp3=tatu.mp3
fi
namebase=`echo "$inmp3" | sed 's|\.mp3$||'`

mpgeditdir=/home/number6/wrk/audio/mpgedit/dev/mpgedit_0-73_dev_work/linux/mpgedit

export LD_LIBRARY_PATH=$mpgeditdir
mpgedit="$mpgeditdir/mpgedit"
cnt=1
outmp3=${namebase}_$cnt.mp3

length=`$mpgedit -I -v $inmp3 | grep 'Track length' | awk '{print $4}' | sed 's/[()s]//g'`
while [ $cnt -lt $length ]; do
  echo $mpgedit -o $outmp3 -I -e$cnt- $inmp3
       $mpgedit -o $outmp3 -I -e$cnt- $inmp3
  cnt=$(($cnt+1))
  outmp3=${namebase}_$cnt.mp3
done

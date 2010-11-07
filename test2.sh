#!/bin/sh
#
# Slice and dice the entire test file into individual frames.
# This test builds a list of edit times, derived from the output of 
# mpgedit -vvv, the end time offset for each frame.  Then each
# frame is edited using these times.  After these edits are complete,
# the individual frame edits are spliced back together, and that
# splice file is diff'ed against the original input file.
#

echon()
{
  if [ $n_flag -eq 1 ]; then
    echo -n "$1"
  else
    echo "$1"'\c'
  fi
}


n_flag=1
[ `uname` = "HP-UX" ] && n_flag=0
[ `uname` = "AIX" ] && n_flag=0

if [ -n "$1" ]; then
  edit_file="$1"
else
  edit_file=test1.mp3
fi
base=`echo $edit_file | sed 's/\..*$//'`
ext=`echo $edit_file | sed 's/.*\.//'`

mpgedit=./mpgedit
rm -f ${base}.idx
rm -f ${base}_*.$ext
rm -f ${base}_*.idx 
rm -f ${base}_splice*.$ext 
rm -f ${base}.times

trap "rm -f ${base}.idx; rm -f  ${base}_*.$ext; rm -f ${base}_*.idx; rm -f ${base}_splice*.$ext; rm -f ${base}.times; echo; exit 1" 1 2 3 15

silent="-s"
tn=$MPGEDIT_TEST_NUMBER
if [ -z "$tn" ]; then
  tn=1
fi

#
# Extract each frame end time from the mpgedit verbose dump. These
# times are used to drive the individual frame edits.
#
$mpgedit -vvv ${base}.$ext | grep 'header t=' | \
  sed -e 's/.*header t=\([0-9][0-9]*\.[0-9][0-9]*\)s.*/\1/' > ${base}.times
total_frames=`$mpgedit ${base}.$ext | \
              grep 'Total frames:' | sed 's/Total frames:  *//'`

echo "Test $tn: Cutting entire ${base}.$ext file into individual frames,"
echo "        then splicing them together again."
echo "        This test takes a long, long time..."
echo
echo "        $total_frames edits are performed by this test"
echon "        working."

t=0
c=0
mod=0
etimes=""
for i in `cat ${base}.times`; do
  etimes="$etimes -e:$t-$i"
  mod=`expr $mod + 1`
  if [ $mod -eq 38 ]; then
    echon "."
    $mpgedit $silent $etimes ${base}.$ext
    etimes=""
    mod=0
  fi
  t=$i
  c=`expr $c + 1`
done
if [ $mod -gt 0 ]; then
  echon "."
  $mpgedit $silent $etimes ${base}.$ext
fi
echo ""

if [ $c -ne $total_frames ]; then
  echo "Error: total number of frames edited '$c'; expected '$total_frames'"
  exit 1
fi

echo "Test $tn: Rejoining $total_frames single frame edits"
echon "        working."

c=1
mod=0
files=""
while [ -f ${base}_$c.$ext  ]; do
  files="$files ${base}_$c.$ext"
  mod=`expr $mod + 1`
  if [ $mod -eq 38 ]; then
    echon "."
    $mpgedit $silent -e- -o +${base}_splice.$ext $files
    files=""
    mod=0
  fi
  c=`expr $c + 1`
done
if [ $mod -gt 0 ]; then
  echon "."
  $mpgedit $silent -e- -o +${base}_splice.$ext $files
fi
echo ""

echo "Test $tn: Comparing spliced file with original file"
cmp ${base}_splice.$ext ${base}.$ext
if [ $? != 0 ]; then
  echo "Failed split/splice test $tn"
  exit 1
fi
echo 
rm -f ${base}.idx
rm -f ${base}_*.$ext
rm -f ${base}_*.idx 
rm -f ${base}_splice*.$ext 
rm -f ${base}.times

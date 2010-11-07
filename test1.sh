#!/bin/sh
# Use /usr/ucb/echo on Solaris for echo -n
PATH=/usr/ucb:$PATH
export PATH
LD_LIBRARY_PATH=`pwd`
export LD_LIBRARY_PATH

silent=-s
#mpgedit=./mpgedit_nocurses
mpgedit=./mpgedit

trap "rm -f test1.idx test1_*.mp3 test1_*.idx test1_splice*.mp3 \
            test2_splice.mp3 test3_splice.mp3 test2_x* test1_x* \
            ATrain_*.mp3 ATrain*.idx ATrain23_?.idx;  echo; exit 1" 1 2 3 15

#
# Test editing entire file into output file; expensive mp3 file copy :/
#
tn=1
echo "Test $tn: Editing entire file into output file"
rm -f test1.idx test1_*.mp3 test1_*.idx test1_splice*.mp3
$mpgedit $silent -e- test1.mp3
echo "Test $tn: Comparing output file with original"
diff test1_1.mp3  test1.mp3
if [ $? != 0 ]; then
  echo "Failed compare test 1"
  exit 1
fi
echo

#
# Test cutting file into four pieces
#
tn=`expr $tn + 1`
echo "Test $tn: Cutting test1.mp3 into 4 pieces"
rm -f test1.idx test1_*.mp3 test1_*.idx test1_splice*.mp3
$mpgedit $silent -e-:6.500 -e:6.500-:12.500 -e:12.500-:19 -e:19- test1.mp3

echo "Test $tn: Splicing 4 pieces together"
$mpgedit $silent -e- -o test1_splice.mp3 test1_1.mp3 test1_2.mp3 \
          test1_3.mp3 test1_4.mp3

echo "Test $tn: Comparing spliced file with original"
diff test1_splice.mp3  test1.mp3
if [ $? != 0 ]; then
  echo "Failed split/splice test $tn"
  exit 1
fi
echo

#
# Test cutting file into 31 1 second pieces
#
tn=`expr $tn + 1`
echo "Test $tn: Cutting test1.mp3 into 31,  1 second segments"
rm -f test1.idx test1_*.mp3 test1_*.idx test1_splice*.mp3
$mpgedit $silent                           \
  -e0-1   -e1-2   -e2-3   -e3-4   -e4-5   \
  -e5-6   -e6-7   -e7-8   -e8-9   -e9-10  \
  -e10-11 -e11-12 -e12-13 -e13-14 -e14-15 \
  -e15-16 -e16-17 -e17-18 -e18-19 -e19-20 \
  -e20-21 -e21-22 -e22-23 -e23-24 -e24-25 \
  -e25-26 -e26-27 -e27-28 -e28-29 -e29-30 \
  -e30- test1.mp3

echo "Test $tn: Splicing 31 pieces together"
$mpgedit $silent -e- -o test1_splice31.mp3                          \
  test1_1.mp3  test1_2.mp3  test1_3.mp3  test1_4.mp3  test1_5.mp3  \
  test1_6.mp3  test1_7.mp3  test1_8.mp3  test1_9.mp3  test1_10.mp3 \
  test1_11.mp3 test1_12.mp3 test1_13.mp3 test1_14.mp3 test1_15.mp3 \
  test1_16.mp3 test1_17.mp3 test1_18.mp3 test1_19.mp3 test1_20.mp3 \
  test1_21.mp3 test1_22.mp3 test1_23.mp3 test1_24.mp3 test1_25.mp3 \
  test1_26.mp3 test1_27.mp3 test1_28.mp3 test1_29.mp3 test1_30.mp3 \
  test1_31.mp3

echo "Test $tn: Comparing spliced file with original"
diff test1_splice31.mp3 test1.mp3
if [ $? != 0 ]; then
  echo "Failed split/splice test $tn"
  exit 1
fi
echo

#
# Test cutting file into 31 1 second pieces, using fractional edit boundaries
#
tn=`expr $tn + 1`
echo "Test $tn: Cutting test1.mp3 into 31, 1 second segments with fractional"
echo "        start and end time"
rm -f test1.idx test1_*.mp3 test1_*.idx test1_splice*.mp3
$mpgedit $silent                                                   \
  -e0-1.50        -e1.50-2.100    -e2.100-3.150   -e3.150-4.200   \
  -e4.200-5.250   -e5.250-6.300   -e6.300-7.350   -e7.350-8.400   \
  -e8.400-9.450   -e9.450-10.500  -e10.500-11.550 -e11.550-12.600 \
  -e12.600-13.650 -e13.650-14.700 -e14.700-15.750 -e15.750-16.800 \
  -e16.800-17.850 -e17.850-18.900 -e18.900-19.950 -e19.950-20.25  \
  -e20.25-21.75   -e21.75-22.125  -e22.125-23.175 -e23.175-24.225 \
  -e24.225-25.275 -e25.275-26.325 -e26.325-27.375 -e27.375-28.425 \
  -e28.425-29.475 -e29.475-30.525 -e30.525- test1.mp3

echo "Test $tn: Splicing 31 pieces together"
$mpgedit $silent -e- -o test1_splice31_2.mp3                        \
  test1_1.mp3  test1_2.mp3  test1_3.mp3  test1_4.mp3  test1_5.mp3  \
  test1_6.mp3  test1_7.mp3  test1_8.mp3  test1_9.mp3  test1_10.mp3 \
  test1_11.mp3 test1_12.mp3 test1_13.mp3 test1_14.mp3 test1_15.mp3 \
  test1_16.mp3 test1_17.mp3 test1_18.mp3 test1_19.mp3 test1_20.mp3 \
  test1_21.mp3 test1_22.mp3 test1_23.mp3 test1_24.mp3 test1_25.mp3 \
  test1_26.mp3 test1_27.mp3 test1_28.mp3 test1_29.mp3 test1_30.mp3 \
  test1_31.mp3

echo "Test $tn: Comparing spliced file with original"
diff test1_splice31_2.mp3 test1.mp3
if [ $? != 0 ]; then
  echo "Failed split/splice test $tn"
  exit 1
fi
echo

#
# Cut first second of test file into individual frames
#
tn=`expr $tn + 1`
rm -f test1.idx test1_*.mp3 test1_*.idx test1_splice*.mp3
echo "Test $tn: Cutting test1.mp3 into 39, 26 millisecond segments"
$mpgedit $silent \
-e0:0.0-0:0.26    -e0:0.26-0:0.52   -e0:0.52-0:0.78   -e0:0.78-0:0.104 \
-e0:0.104-0:0.130 -e0:0.130-0:0.156 -e0:0.156-0:0.182 -e0:0.182-0:0.208 \
-e0:0.208-0:0.234 -e0:0.234-0:0.260 -e0:0.260-0:0.286 -e0:0.286-0:0.312 \
-e0:0.312-0:0.338 -e0:0.338-0:0.364 -e0:0.364-0:0.390 -e0:0.390-0:0.416 \
-e0:0.416-0:0.442 -e0:0.442-0:0.468 -e0:0.468-0:0.494 -e0:0.494-0:0.520 \
-e0:0.520-0:0.546 -e0:0.546-0:0.572 -e0:0.572-0:0.598 -e0:0.598-0:0.624 \
-e0:0.624-0:0.650 -e0:0.650-0:0.676 -e0:0.676-0:0.702 -e0:0.702-0:0.728 \
-e0:0.728-0:0.754 -e0:0.754-0:0.780 -e0:0.780-0:0.806 -e0:0.806-0:0.832 \
-e0:0.832-0:0.858 -e0:0.858-0:0.884 -e0:0.884-0:0.910 -e0:0.910-0:0.936 \
-e0:0.936-0:0.962 -e0:0.962-0:0.988 -e0:0.988-0:0.1014 test1.mp3


echo "Test $tn: Cutting a 1 second segment of test1.mp3"
$mpgedit $silent -e0-1 test1.mp3

echo "Test $tn: Splicing 39 pieces together"
$mpgedit $silent -o test1_splice38.mp3                   \
-e- test1_1.mp3  test1_2.mp3  test1_3.mp3               \
    test1_4.mp3  test1_5.mp3  test1_6.mp3  test1_7.mp3  \
    test1_8.mp3  test1_9.mp3  test1_10.mp3 test1_11.mp3 \
    test1_12.mp3 test1_13.mp3 test1_14.mp3 test1_15.mp3 \
    test1_16.mp3 test1_17.mp3 test1_18.mp3 test1_19.mp3 \
    test1_20.mp3 test1_21.mp3 test1_22.mp3 test1_23.mp3 \
    test1_24.mp3 test1_25.mp3 test1_26.mp3 test1_27.mp3 \
    test1_28.mp3 test1_29.mp3 test1_30.mp3 test1_31.mp3 \
    test1_32.mp3 test1_33.mp3 test1_34.mp3 test1_35.mp3 \
    test1_36.mp3 test1_37.mp3 test1_38.mp3 test1_39.mp3 

echo "Test $tn: Comparing spliced file with 1 second segment"
diff test1_40.mp3 test1_splice38.mp3
if [ $? != 0 ]; then
  echo "Failed split/splice test $tn"
  exit 1
fi
rm -f test1.idx test1_*.mp3 test1_*.idx test1_splice*.mp3
echo 

#
# Slice and dice test
#
tn=`expr $tn + 1`
MPGEDIT_TEST_NUMBER=$tn
export MPGEDIT_TEST_NUMBER
./test2.pl test1.mp3
[ $? -ne 0 ] && exit 1

#
# Slice and dice test with source file ending on an odd millisecond boundary
#
tn=`expr $tn + 1`
MPGEDIT_TEST_NUMBER=$tn
export MPGEDIT_TEST_NUMBER
rm -f test1.idx test1_*.mp3 test1_*.idx test1_splice*.mp3
echo "Test $tn: Editing a 6.555s segment from test1.mp3"
echo "        This tests editing a file ending on an odd millisecond boundary"
$mpgedit $silent -e-:6.555 test1.mp3
echo "Test $tn: Cutting segment test1_1.mp3 into individual frames"
./test2.pl test1_1.mp3
[ $? -ne 0 ] && exit 1
rm -f test1.idx test1_*.mp3 test1_*.idx test1_splice*.mp3
echo

#
# Cut CBR encoded file with EOF not on an even frame boundary in
# two pieces, then join
tn=`expr $tn + 1`
rm -f test2.idx test2_*.mp3 test2_*.idx test2_splice.mp3 test3_splice.mp3
echo "Test $tn: Editing CBR file with EOF not on an even frame boundary"
$mpgedit $silent -e-3.555 -e3.555- test2.mp3
#
# These two command lines are the same, but the first one used to crash
# while the second worked, so test both here
#
$mpgedit $silent -f test2_1.mp3 -f test2_2.mp3 -e-  -o test2_splice.mp3
$mpgedit $silent -o test3_splice.mp3 -e- test2_1.mp3 test2_2.mp3

diff test2.mp3 test2_splice.mp3
if [ $? != 0 ]; then
  echo "Failed test $tn"
  exit 1
fi
diff test2.mp3 test3_splice.mp3
if [ $? != 0 ]; then
  echo "Failed test $tn"
  exit 1
fi
rm -f test2.idx test2_*.mp3 test2_*.idx test2_splice.mp3 test3_splice.mp3
echo

#
# Slice and dice test using CBR encoded file, and EOF is not on an even
# frame boundary.
#
tn=`expr $tn + 1`
echo "Test $tn: Slice and dice test using CBR encoded file, and EOF is"
echo "        not on an even frame boundary."
MPGEDIT_TEST_NUMBER=$tn
export MPGEDIT_TEST_NUMBER
./test2.pl test2.mp3
[ $? -ne 0 ] && exit 1
echo 

#
# Test CBR encoded file with zero junk at head of file is skipped, and
# ID3 tag at end of file is ignored.
#
tn=`expr $tn + 1`
echo "Test $tn: Constructing CBR file, with ID3 tag and prefixed with zeros"
rm -f test2_x*
cp zero_prefix test2_x.mp3
cat test2.mp3 >> test2_x.mp3
cat test2.tag >> test2_x.mp3
echo "Test $tn: $mpgedit edit of entire file should remove prefix and ID3 tag"
$mpgedit $silent -e- test2_x.mp3
echo "Test $tn: Comparing edited file with original unmodified file"
diff test2.mp3 test2_x_1.mp3
if [ $? != 0 ]; then
  echo "Failed test $tn"
  exit 1
fi
rm -f test2_x*
echo


#
# Test VBR encoded file with zero junk at head of file is skipped, and
# ID3 tag at end of file is ignored.
#
tn=`expr $tn + 1`
echo "Test $tn: Constructing VBR file, with ID3 tag and prefixed with zeros"
rm -f test1_x*
cp zero_prefix test1_x.mp3
cat test1.mp3 >> test1_x.mp3
cat test2.tag >> test1_x.mp3
echo "Test $tn: $mpgedit edit of entire file should remove prefix and ID3 tag"
$mpgedit $silent -e- test1_x.mp3
echo "Test $tn: Comparing edited file with original unmodified file"
diff test1.mp3 test1_x_1.mp3
if [ $? != 0 ]; then
  echo "Failed test $tn"
  exit 1
fi
rm -f test1_x*
echo

#
# Test CBR encoded file with random junk at head of file is skipped, and
# ID3 tag at end of file is ignored.
#
tn=`expr $tn + 1`
echo "Test $tn: Constructing CBR file, with ID3 tag and random data prefix"
rm -f test2_x*
cp random_prefix test2_x.mp3
cat test2.mp3 >> test2_x.mp3
cat test2.tag >> test2_x.mp3
echo "Test $tn: $mpgedit edit of entire file should remove prefix and ID3 tag"
$mpgedit $silent -e- test2_x.mp3
echo "Test $tn: Comparing edited file with original unmodified file"
diff test2.mp3 test2_x_1.mp3
if [ $? != 0 ]; then
  echo "Failed test $tn"
  exit 1
fi
rm -f test2_x*
echo

#
# Test CBR encoded file with junk data at head of file is skipped, and
# ID3 tag at end of file is ignored.
#
tn=`expr $tn + 1`
echo "Test $tn: Constructing CBR file, with ID3 tag and junk data prefix"
rm -f test2_x*
cp ones_data test2_x.mp3
cat test2.mp3 >> test2_x.mp3
cat test2.tag >> test2_x.mp3
echo "Test $tn: $mpgedit edit of entire file should remove prefix and ID3 tag"
$mpgedit $silent -e- test2_x.mp3
echo "Test $tn: Comparing edited file with original unmodified file"
diff test2.mp3 test2_x_1.mp3
if [ $? != 0 ]; then
  echo "Failed test $tn"
  exit 1
fi
rm -f test2_x*
echo

#
# Test VBR encoded file with random junk at head of file is skipped, and
# ID3 tag at end of file is ignored.
#
tn=`expr $tn + 1`
echo "Test $tn: Constructing VBR file, with ID3 tag and random data prefix"
rm -f test1_x*
cp random_prefix test1_x.mp3
cat test1.mp3 >> test1_x.mp3
cat test2.tag >> test1_x.mp3
echo "Test $tn: $mpgedit edit of entire file should remove prefix and ID3 tag"
$mpgedit $silent -e- test1_x.mp3
echo "Test $tn: Comparing edited file with original unmodified file"
diff test1.mp3 test1_x_1.mp3
if [ $? != 0 ]; then
  echo "Failed test $tn"
  exit 1
fi
rm -f test1_x*
echo

#
# Test VBR encoded file with junk data at head of file is skipped, and
# ID3 tag at end of file is ignored.
#
tn=`expr $tn + 1`
echo "Test $tn: Constructing VBR file, with ID3 tag and junk data prefix"
rm -f test1_x*
cp ones_data test1_x.mp3
cat test1.mp3 >> test1_x.mp3
cat test2.tag >> test1_x.mp3
echo "Test $tn: $mpgedit edit of entire file should remove prefix and ID3 tag"
$mpgedit $silent -e- test1_x.mp3
echo "Test $tn: Comparing edited file with original unmodified file"
diff test1.mp3 test1_x_1.mp3
if [ $? != 0 ]; then
  echo "Failed test $tn"
  exit 1
fi
rm -f test1_x*
echo

#
# Test VBR encoded file with random junk at head of file is skipped, and
# ID3 tag at end of file is ignored.
#
tn=`expr $tn + 1`
echo "Test $tn: Constructing VBR file, with ID3 tag and junk data prefix"
rm -f test1_x*
cp  ones_data test1_x.mp3
cat ones_data >> test1_x.mp3
cat ones_data >> test1_x.mp3
cat ones_data >> test1_x.mp3
cat ones_data >> test1_x.mp3
cat ones_data >> test1_x.mp3

cat test1.mp3 >> test1_x.mp3

cat test2.tag >> test1_x.mp3
cat ones_data >> test1_x.mp3
cat ones_data >> test1_x.mp3
cat ones_data >> test1_x.mp3
cat ones_data >> test1_x.mp3
cat ones_data >> test1_x.mp3
cat ones_data >> test1_x.mp3

echo "Test $tn: $mpgedit edit of entire file should remove prefix and ID3 tag"
$mpgedit $silent -e- test1_x.mp3
echo "Test $tn: Comparing edited file with original unmodified file"
diff test1.mp3 test1_x_1.mp3
if [ $? != 0 ]; then
  echo "Failed test $tn"
  exit 1
fi
rm -f test1_x*
echo

#
# Test editing of MPEG 2 Layer 3 encoded file.  Just cut entire file
# and compare against original.  Remember, must remove ID3 tag before
# comparison, as mpgedit also strips the ID3 tag.
#
if [ -f ATrain.mp3 ]; then
  rm -f ATrain_*.mp3 ATrain*.idx
  tn=`expr $tn + 1`
  echo "Test $tn: Editing entire MPEG 2 layer 3 file"
  len=`wc -c ATrain.mp3 | awk '{print $1}'`
  len_noid3=`expr $len - 128`

  echo "Test $tn: Stripping ID3 tag from source file"
  dd bs=$len_noid3 count=1 if=ATrain.mp3 of=ATrain_noid3.mp3

  echo "Test $tn: Cutting entire MPEG 2 file"
  $mpgedit $silent -e- ATrain.mp3

  echo "Test $tn: Comparing cut file with original file"
  cmp ATrain_1.mp3 ATrain_noid3.mp3
  if [ $? != 0 ]; then
    echo "Failed test $tn"
    exit 1
  fi
  rm -f ATrain_*.mp3 ATrain*.idx
  echo
fi

#
# Edit entire MPEG 2 file
#
tn=`expr $tn + 1`
rm -f ATrain23_*.mp3 ATrain23*.idx
echo "Test $tn: Editing entire MPEG 2 layer 3 file"
$mpgedit $silent -e- ATrain23.mp3

echo "Test $tn: Comparing cut file with original file"
cmp ATrain23_1.mp3 ATrain23.mp3
if [ $? != 0 ]; then
  echo "Failed test $tn"
  exit 1
fi
rm -f ATrain23_*.mp3 ATrain23*.idx
echo 

#
# Edit MPEG 2 file into multiple pieces
#
tn=`expr $tn + 1`
rm -f ATrain23_*.mp3 ATrain23.idx ATrain23_?.idx
echo "Test $tn: Cutting ATrain23.mp3 into 8 pieces"
$mpgedit $silent -e-3.33   -e3.33-6.66 -e6.66-9.99 -e9.99-12 \
                 -e12-15.1 -e15.1-18.5 -e18.5-21   -e21- ATrain23.mp3

echo "Test $tn: Splicing 8 pieces together"
$mpgedit $silent -o ATrain23_splice.mp3 -e-                    \
  ATrain23_1.mp3 ATrain23_2.mp3 ATrain23_3.mp3 ATrain23_4.mp3  \
  ATrain23_5.mp3 ATrain23_6.mp3 ATrain23_7.mp3 ATrain23_8.mp3 

echo "Test $tn: Comparing cut file with original file"
cmp ATrain23_splice.mp3 ATrain23.mp3
if [ $? != 0 ]; then
  echo "Failed test $tn"
  exit 1
fi
rm -f ATrain23_*.mp3 ATrain23.idx ATrain23_?.idx
echo

#
# Chop MPEG 2 file into individual frames
#
tn=`expr $tn + 1`
echo "Test $tn: Cutting entire MPEG 2 layer 3 file into individual frames"
MPGEDIT_TEST_NUMBER=$tn
export MPGEDIT_TEST_NUMBER
./test2.pl ATrain23.mp3
[ $? -ne 0 ] && exit 1

#
# Call MPEG1 layer 1/2/3, MPEG2 layer 2/3 parsing test script
#
tn=`expr $tn + 1`
echo "Test $tn: Reading MPEG layer 1/2/3, MPEG2 layer 2/3 files"
echo "This checks the ability of mpgedit to properly parse these file formats"
./test4.pl
status=$?
[ $status -ne  0 ] && exit 1
echo "Test $tn: Completed successfully"

#
# Call VBR test script, test3.pl
#
tn=`expr $tn + 1`
echo "Test $tn: Edit MP3/MPEG2/MPEG2.5 VBR files, and verify Xing headers"
./test3.pl
status=$?
[ $status -ne  0 ] && exit 1
echo "Test $tn: Completed successfully"

#
# Add more tests here
#
exit 0

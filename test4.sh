#!/bin/sh
#
# Test ability to parse mpeg1 layer 1/2/3 files, and mpeg2 layer 2/3 files.
# No mpeg2 layer 1 test files are available at this time.
#
# mpg1_layer1_fl1.mp3    - MPEG1 layer 1 file br=384 kbps/sr=32KHz
# mpg1_layer1_fl7.mp3    - MPEG1 layer 1 file br=384 kbps/sr=44.1KHz
# mpg1_layer2_fl11.mp3   - MPEG1 layer 2 file br=192 kbps/sr=44.1KHz
# mpg1_layer2_fl16.mp3   - MPEG1 layer 2 file br=256 kbps/sr=48KHz
# mpg2_layer2_test24.mp3 - MPEG2 layer 2 file br=96  kbps/sr=16KHz
# test1.mp3              - MPEG1 layer 3 file br=VBR     /sr=44.1KHz
# ATrain23.mp3           - MPEG2 layer 3 file br=64  kbps/sr=22.05KHz
#
# Earlier releases of mpgedit fail these tests.  MPEG1 layer 2/3, and
# mostly MPEG2 layer 3 worked since 0.6 and earlier.
#
# This test is heavily dependent on the summary statistics printed
# after mpgedit has finished processing the input file.  Any changes  to
# the format of this output may break this script.

#mpgedit=$HOME/wrk/mpgedit/production/mpgedit_0.5_linux/mpgedit
#mpgedit=$HOME/wrk/mpgedit/production/mpgedit_0.6/linux/mpgedit
mpgedit=./mpgedit
errcnt=0

rm -f  ATrain23.idx  mpg1_layer1_fl7.idx   mpg1_layer2_fl16.idx  \
       test1.idx     mpg1_layer1_fl1.idx   mpg1_layer2_fl11.idx  \
       mpg2_layer2_test24.idx

trap "rm -f ATrain23.idx  mpg1_layer1_fl7.idx   mpg1_layer2_fl16.idx  \
            test1.idx     mpg1_layer1_fl1.idx   mpg1_layer2_fl11.idx  \
            mpg2_layer2_test24.idx; exit 1" 1 2 3 15

#
# These are the correct answers for mpg1_layer1_fl1.mp3
#
# File name:    mpg1_layer1_fl1.mp3
# CBR:          384
# Total frames: 49
# File size:    28224
# Track length: 0:00.588 (0s)
#

err=0
file=mpg1_layer1_fl1.mp3
out=`$mpgedit $file`

echo "Parsing MPEG1 layer1 file"
echo "testing $file"
tmptst=`echo "$out" | grep 'CBR:' | sed 's/CBR:  *//'`
[ "$tmptst" != "384" ] && echo "$file: bitrate incorrect '$tmptst'" && err=1
tmptst=`echo "$out" | grep 'Total frames:' | sed 's/Total frames:  *//'`
[ "$tmptst" != "49" ] && echo "$file: frame count incorrect '$tmptst'" && err=1
tmptst=`echo "$out" | grep 'File size:' | sed 's/File size:  *//'`
[ "$tmptst" != "28224" ] && echo "$file: file size incorrect '$tmptst'" && err=1
[ $err -eq 1 ] && errcnt=`expr $errcnt + 1`
echo ""

#
# These are the correct answers for mpg1_layer1_fl7.mp3
#
# File name:    mpg1_layer1_fl7.mp3
# CBR:          384
# Total frames: 63
# File size:    26332
# Track length: 0:00.548 (0s)

err=0
file=mpg1_layer1_fl7.mp3
out=`$mpgedit $file`

echo "Parsing MPEG1 layer 1 file"
echo "testing $file"
tmptst=`echo "$out" | grep 'CBR:' | sed 's/CBR:  *//'`
[ "$tmptst" != "384" ] && echo "$file: bitrate incorrect '$tmptst'" && err=1
tmptst=`echo "$out" | grep 'Total frames:' | sed 's/Total frames:  *//'`
[ "$tmptst" != "63" ] && echo "$file: frame count incorrect '$tmptst'" && err=1
tmptst=`echo "$out" | grep 'File size:' | sed 's/File size:  *//'`
[ "$tmptst" != "26332" ] && echo "$file: file size incorrect '$tmptst'" && err=1
[ $err -eq 1 ] && errcnt=`expr $errcnt + 1`
echo ""

#
# These are the correct answers for mpg1_layer2_fl11.mp3
#
# File name:    mpg1_layer2_fl11.mp3
# CBR:          192
# Total frames: 49
# File size:    30720
# Track length: 0:01.279 (1s)

err=0
file=mpg1_layer2_fl11.mp3
out=`$mpgedit $file`

echo "Parsing MPEG1 layer 2 file"
echo "testing $file"
tmptst=`echo "$out" | grep 'CBR:' | sed 's/CBR:  *//'`
[ "$tmptst" != "192" ] && echo "$file: bitrate incorrect '$tmptst'" && err=1
tmptst=`echo "$out" | grep 'Total frames:' | sed 's/Total frames:  *//'`
[ "$tmptst" != "49" ] && echo "$file: frame count incorrect '$tmptst'" && err=1
tmptst=`echo "$out" | grep 'File size:' | sed 's/File size:  *//'`
[ "$tmptst" != "30720" ] && echo "$file: file size incorrect '$tmptst'" && err=1
[ $err -eq 1 ] && errcnt=`expr $errcnt + 1`
echo ""


#
# These are the correct answers for mpg1_layer2_fl16.mp3
#
# File name:    mpg1_layer2_fl16.mp3
# CBR:          256
# Total frames: 63
# File size:    48384
# Track length: 0:01.512 (1s)

err=0
file=mpg1_layer2_fl16.mp3
out=`$mpgedit $file`

echo "Parsing MPEG1 layer 2 file"
echo "testing $file"
tmptst=`echo "$out" | grep 'CBR:' | sed 's/CBR:  *//'`
[ "$tmptst" != "256" ] && echo "$file: bitrate incorrect '$tmptst'" && err=1
tmptst=`echo "$out" | grep 'Total frames:' | sed 's/Total frames:  *//'`
[ "$tmptst" != "63" ] && echo "$file: frame count incorrect '$tmptst'" && err=1
tmptst=`echo "$out" | grep 'File size:' | sed 's/File size:  *//'`
[ "$tmptst" != "48384" ] && echo "$file: file size incorrect '$tmptst'" && err=1
[ $err -eq 1 ] && errcnt=`expr $errcnt + 1`
echo ""

#
# These are the correct answers for mpg2_layer2_test24.mp3
#
# File name:    mpg2_layer2_test24.mp3
# CBR:          96
# Total frames: 27
# File size:    23328
# Track length: 0:00.972 (0s)

err=0
file=mpg2_layer2_test24.mp3
out=`$mpgedit $file`

echo "Parsing MPEG2 layer 2 file"
echo "testing $file"
tmptst=`echo "$out" | grep 'CBR:' | sed 's/CBR:  *//'`
[ "$tmptst" != "96" ] && echo "$file: bitrate incorrect '$tmptst'" && err=1
tmptst=`echo "$out" | grep 'Total frames:' | sed 's/Total frames:  *//'`
[ "$tmptst" != "27" ] && echo "$file: frame count incorrect '$tmptst'" && err=1
tmptst=`echo "$out" | grep 'File size:' | sed 's/File size:  *//'`
[ "$tmptst" != "23328" ] && echo "$file: file size incorrect '$tmptst'" && err=1
[ $err -eq 1 ] && errcnt=`expr $errcnt + 1`
echo ""


#
# These are the correct answers for mpg2_layer2_test24.mp3
#
# File name:    test1.mp3
# VBR Min:      48
# VBR Max:      224
# VBR Average:  144
# Total frames: 1264
# File size:    596332
# Track length: 0:33.18 (33s)

err=0
file=test1.mp3
out=`$mpgedit $file`

echo "Parsing MPEG1 layer 3 file"
echo "testing $file"
tmptst=`echo "$out" | grep 'VBR Min:' | sed 's/VBR Min:  *//'`
[ "$tmptst" != "48" ] && echo "$file: VBR min incorrect '$tmptst'" && err=1
tmptst=`echo "$out" | grep 'VBR Max:' | sed 's/VBR Max:  *//'`
[ "$tmptst" != "224" ] && echo "$file: VBR max incorrect '$tmptst'" && err=1
tmptst=`echo "$out" | grep 'VBR Average:' | sed 's/VBR Average:  *//'`
[ "$tmptst" != "144" ] && echo "$file: VBR average incorrect '$tmptst'" && err=1
tmptst=`echo "$out" | grep 'Total frames:' | sed 's/Total frames:  *//'`
[ "$tmptst" != "1264" ] && \
    echo "$file: frame count incorrect '$tmptst'" && err=1
tmptst=`echo "$out" | grep 'File size:' | sed 's/File size:  *//'`
[ "$tmptst" != "596332" ] && \
    echo "$file: file size incorrect '$tmptst'" && err=1
[ $err -eq 1 ] && errcnt=`expr $errcnt + 1`
echo ""


#
# These are the correct answers for ATrain23.mp3
#
# CBR:          64
# Total frames: 896
# File size:    187245
# Track length: 0:23.405 (23s)

err=0
file=ATrain23.mp3
out=`$mpgedit $file`

echo "Parsing MPEG2 layer 3 file"
echo "testing $file"
tmptst=`echo "$out" | grep 'CBR:' | sed 's/CBR:  *//'`
[ "$tmptst" != "64" ] && echo "$file: bitrate incorrect '$tmptst'" && err=1
tmptst=`echo "$out" | grep 'Total frames:' | sed 's/Total frames:  *//'`
[ "$tmptst" != "896" ] && echo "$file: frame count incorrect '$tmptst'" && err=1
tmptst=`echo "$out" | grep 'File size:' | sed 's/File size:  *//'`
[ "$tmptst" != "187245" ] && \
    echo "$file: file size incorrect '$tmptst'" && err=1
[ $err -eq 1 ] && errcnt=`expr $errcnt + 1`
echo ""

if [ $errcnt -eq 0 ]; then
    echo "SUCCESS: Correctly parsed all input files"
else
    echo "ERROR: Failed to correctly parse $errcnt files"
fi

# clean up the mess we just created
rm -f  ATrain23.idx  mpg1_layer1_fl7.idx   mpg1_layer2_fl16.idx  \
       test1.idx     mpg1_layer1_fl1.idx   mpg1_layer2_fl11.idx  \
       mpg2_layer2_test24.idx

exit $errcnt

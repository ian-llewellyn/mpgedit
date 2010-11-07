#!/bin/sh

test_statistics()
{
    mpgedit="$1"
    xing="$2"
    name="$3"

    return_test_statistics=0
    echo ""
    echo "Testing Xing header for '$name'"
    mpgedit_frames=`echo "$mpgedit" | grep '^Total frames:' |
                          sed 's/^Total frames: *//'`
    mpgedit_bytes=`echo  "$mpgedit" | grep '^File size:' |
                          sed 's/File size: *//'`
    xing_frames=`echo    "$xing" | grep '^frames *=' |
                          sed 's/frames *= *//'`
    xing_bytes=`echo     "$xing" | grep '^bytes *= *' |
                         sed 's/bytes *= *//'`
    if [ $mpgedit_frames -ne $xing_frames ]; then
        echo "ERROR: $name frames not the same ($mpgedit_frames:$xing_frames)" 
        return_test_statistics=1
    elif [ $mpgedit_bytes -ne $xing_bytes ]; then
        echo "ERROR: $name bytes not the same ($mpgedit_bytes:$xing_bytes)" 
        return_test_statistics=1
    else
        echo "mpgedit: fr=$mpgedit_frames bytes=$mpgedit_bytes"
        echo "vbr:     fr=$xing_frames bytes=$xing_bytes"
    fi

    unset mpgedit xing name mpgedit_frames 
    unset mpgedit_bytes xing_frames xing_bytes 
}


#
# Test Xing headers in VBR files
#
mpgedit=./mpgedit
lame=`lame --help 2>&1`
sox=`sox -h 2>&1`
[ -z "`echo $lame | grep '^LAME'`" ] && echo "'lame' must be installed to proceed" && exit 0
[ -z "`echo $sox  | grep -i 'usage:'`" ] && echo "'sox' must be installed to proceed" && exit 0
trap "rm -f test1_* test1.wav test1.idx; exit 1" 1 2 3 15

echo ""
echo "
         This test verifies the Xing header present in VBR encoded files is
         properly modified after performing edits using mpgedit.  Since the
         data offsets in the Xing header are different for MP3/MPEG2/MPEG2.5
         stereo/mono files, this test edits all of these file types, then
         verifies the Xing header.  To minimize the number of actual test
         data files, all of the test data are derived from test1.mp3.  This
         is accomplished by decoding the MP3 file to a wav file, then down
         sampling the wav file using sox to 22050Hz and 11025Hz sampled wav
         files.  These synthesized wav files are then MP3/MPEG2/MPEG2.5
         encoded with lame.  Afterwards, these newly encoded files are
         edited with mpgedit.  Finally, these edited file's Xing headers are
         verified for correctness.

         All of this file decoding, format conversion, and re-encoding
         makes this test take a very long time to complete.
"
echo ""

# Decode test1.mp3 to a wave file.
echo "Decoding test1.mp3 -> test1.wav"
echo ""
[ -f test1.wav ] || lame -S --decode test1.mp3 test1.wav
[ ! -f test1.wav ] && echo "lame failed to decode test1.mp3" && exit 1

# Lowering sample rate to create MPEG2 and MPEG2.5 files
echo "Converting test1.wav to 44100Hz mono..."
[ -f test1_44100m.wav ] || sox test1.wav          -c 1 test1_44100m.wav
echo "Converting test1.wav to 22050Hz stereo..."
[ -f test1_22050s.wav ] || sox test1.wav -r 22050      test1_22050s.wav
echo "Converting test1.wav to 11025Hz stereo..."
[ -f test1_11025s.wav ] || sox test1.wav -r 11025      test1_11025s.wav
echo "Converting test1.wav to 22050Hz mono..."
[ -f test1_22050m.wav ] || sox test1.wav -r 22050 -c 1 test1_22050m.wav
echo "Converting test1.wav to 11025Hz mono..."
[ -f test1_11025m.wav ] || sox test1.wav -r 11025 -c 1 test1_11025m.wav

# Create MPEG2 encodings of this test file
echo ""
echo "MPEG1 Layer3 VBR encoding 44100Hz mono wav file..."
echo ""
[ -f test1_44100m.mp3 ] || lame -V 1 -S test1_44100m.wav test1_44100m.mp3
echo ""
echo "MPEG2 Layer3 VBR encoding 22050Hz stereo wav file..."
echo ""
[ -f test1_22050s.mp3 ] || lame -V 1 -S test1_22050s.wav test1_22050s.mp3
echo ""
echo "MPEG2.5 Layer 3 VBR encoding 11025Hz stereo wav file..."
echo ""
[ -f test1_11025s.mp3 ] || lame -V 1 -S test1_11025s.wav test1_11025s.mp3
echo ""
echo "MPEG2 Layer 3 VBR encoding 22050Hz mono wav file..."
echo ""
[ -f test1_22050m.mp3 ] || lame -V 1 -S test1_22050m.wav test1_22050m.mp3
echo ""
echo "MPEG2.5 Layer 3 VBR encoding 11025Hz mono wav file..."
echo ""
[ -f test1_11025m.mp3 ] || lame -V 1 -S test1_11025m.wav test1_11025m.mp3

# Perform edits on MPEG2/2.5 files
echo "Editing 6 seconds segments from MPEG2/2.5 files"
[ -f test1_44100m_1.mp3 ] || $mpgedit -s -e:6.500-:12.500 test1_44100m.mp3
[ -f test1_22050s_1.mp3 ] || $mpgedit -s -e:6.500-:12.500 test1_22050s.mp3
[ -f test1_11025s_1.mp3 ] || $mpgedit -s -e:6.500-:12.500 test1_11025s.mp3
[ -f test1_22050m_1.mp3 ] || $mpgedit -s -e:6.500-:12.500 test1_22050m.mp3
[ -f test1_11025m_1.mp3 ] || $mpgedit -s -e:6.500-:12.500 test1_11025m.mp3
[ -f test1_44100s_1.mp3 ] || \
              $mpgedit -s -e:6.500-:12.500 -o test1_44100s_1.mp3 test1.mp3 

# Get mpgedit frame statistics
echo "Reading file statistics using mpgedit"
test1_44100s=`mpgedit -s test1.mp3`
test1_44100m=`mpgedit -s test1_44100m.mp3`
test1_22050s=`mpgedit -s test1_22050s.mp3`
test1_11025s=`mpgedit -s test1_11025s.mp3`
test1_22050m=`mpgedit -s test1_22050m.mp3`
test1_11025m=`mpgedit -s test1_11025m.mp3`

test1_44100s_1=`mpgedit -s test1_44100s_1.mp3`
test1_44100m_1=`mpgedit -s test1_44100m_1.mp3`
test1_22050s_1=`mpgedit -s test1_22050s_1.mp3`
test1_11025s_1=`mpgedit -s test1_11025s_1.mp3`
test1_22050m_1=`mpgedit -s test1_22050m_1.mp3`
test1_11025m_1=`mpgedit -s test1_11025m_1.mp3`

# Read VBR headers with mp3_vbrpatch
echo "Reading file statistics from Xing header"
xingtest1_44100s=`mp3_vbrpatch test1.mp3`
xingtest1_44100m=`mp3_vbrpatch test1_44100m.mp3`
xingtest1_22050s=`mp3_vbrpatch test1_22050s.mp3`
xingtest1_11025s=`mp3_vbrpatch test1_11025s.mp3`
xingtest1_22050m=`mp3_vbrpatch test1_22050m.mp3`
xingtest1_11025m=`mp3_vbrpatch test1_11025m.mp3`

xingtest1_44100s_1=`mp3_vbrpatch test1_44100s_1.mp3`
xingtest1_44100m_1=`mp3_vbrpatch test1_44100m_1.mp3`
xingtest1_22050s_1=`mp3_vbrpatch test1_22050s_1.mp3`
xingtest1_11025s_1=`mp3_vbrpatch test1_11025s_1.mp3`
xingtest1_22050m_1=`mp3_vbrpatch test1_22050m_1.mp3`
xingtest1_11025m_1=`mp3_vbrpatch test1_11025m_1.mp3`

# Test unedited file Xing headers for accuracy
echo ""
echo "Verifying Xing header data for accuracy"
echo ""
errors=0
test_statistics "$test1_44100s" "$xingtest1_44100s" "test1.mp3"
errors=`expr $errors + $return_test_statistics`

test_statistics "$test1_44100m" "$xingtest1_44100m" "test1_44100m.mp3"
errors=`expr $errors + $return_test_statistics`

test_statistics "$test1_22050s" "$xingtest1_22050s" "test1_22050s.mp3"
errors=`expr $errors + $return_test_statistics`

test_statistics "$test1_11025s" "$xingtest1_11025s" "test1_11025s.mp3"
errors=`expr $errors + $return_test_statistics`

test_statistics "$test1_22050m" "$xingtest1_22050m" "test1_22050m.mp3"
errors=`expr $errors + $return_test_statistics`

test_statistics "$test1_11025m" "$xingtest1_11025m" "test1_11025m.mp3"
errors=`expr $errors + $return_test_statistics`

# Test edited file Xing headers for accuracy
test_statistics "$test1_44100s_1" "$xingtest1_44100s_1" "test1_44100s_1.mp3"
errors=`expr $errors + $return_test_statistics`

test_statistics "$test1_44100m_1" "$xingtest1_44100m_1" "test1_44100m_1.mp3"
errors=`expr $errors + $return_test_statistics`

test_statistics "$test1_22050s_1" "$xingtest1_22050s_1" "test1_22050s_1.mp3"
errors=`expr $errors + $return_test_statistics`

test_statistics "$test1_11025s_1" "$xingtest1_11025s_1" "test1_11025s_1.mp3"
errors=`expr $errors + $return_test_statistics`

test_statistics "$test1_22050m_1" "$xingtest1_22050m_1" "test1_22050m_1.mp3"
errors=`expr $errors + $return_test_statistics`

test_statistics "$test1_11025m_1" "$xingtest1_11025m_1" "test1_11025m_1.mp3"
errors=`expr $errors + $return_test_statistics`

if [ $errors -gt 0 ]; then
    echo ""
    echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
    echo "ERROR: $errors tests failed"
    echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
    echo ""
else
    echo ""
    echo "SUCCESS: All tests passed"
    echo ""
fi

rm -f test1_* test1.wav test1.idx
exit $errors

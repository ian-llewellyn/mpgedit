#!/bin/sh
#
# Script to create HTML pages from man pages.  This now exists
# to remove the CGI-BIN remnants.
#
dir=./html
[ -d $dir ] || mkdir $dir

# The sed script
cat << NNNN > sedit
/<A HREF="http:\/\/localhost\.">/{
 s|<A HREF="http://localhost\.">||
 s|</A>||
}
s|http://localhost\.?[0-9]+||g
s|HREF="mpgedit"|HREF="mpgedit.html"|
s|HREF="xmpgedit"|HREF="xmpgedit.html"|
s|HREF="mp3decoder\.sh"|HREF="mp3decoder_sh.html"|
s|HREF="decoder\.so"|HREF="decoder_so.html"|
s|HREF="decoder_popen\.so"|HREF="decoder_so.html"|
s|HREF="decoder_mpg123\.so"|HREF="decoder_so.html"|
s|HREF="popen"|HREF="http://bama.ua.edu/cgi-bin/man-cgi?popen+3C"|
s|HREF="scramble_times\.pl"|HREF="scramble_times_pl.html"|
s|HREF="scramble\.pl"|HREF="scramble_pl.html"|
s|HREF="unscramble\.pl"|HREF="unscramble_pl.html"|

NNNN

man2html -M . mpgedit.1        | sed -f sedit > $dir/mpgedit.html
man2html -M . xmpgedit.1       | sed -f sedit > $dir/xmpgedit.html
man2html -M . decoder.so.1     | sed -f sedit > $dir/decoder_so.html
man2html -M . mp3decoder.sh.1  | sed -f sedit > $dir/mp3decoder_sh.html
man2html -M . scramble_times.pl.1 | sed -f sedit > $dir/scramble_times_pl.html
man2html -M . scramble.pl.1    | sed -f sedit > $dir/scramble_pl.html
man2html -M . unscramble.pl.1  | sed -f sedit > $dir/unscramble_pl.html
rm -f sedit

#!/bin/sh
#
# Script to format the Mac OS X ReadME.html file from
# the standard NEWS file.
#
# Make sed script cat->NNNN
#
cat<<NNNN>sedit
1i\\
<html> \\
These are the latest changes for this release of MPGEDIT. To see the complete \\
change log for this and previous releases of MPGEDIT, see the \\
<a href="http:\/\/mpgedit.org\/mpgedit\/NEWS_0_7.html">MPGEDIT Change Log<\/a>
s/^  *- /<li> /
s/^\(\*.*\*$\)/<\/ul><p> \1 <ul>/
\$a\\
<\/ul> \\
<\/html>
1,/	          	/p
NNNN

#
# Prints the output up to \\t          \t'
#

sed -n -f sedit NEWS | sed '1,/<ul>/s/<\/ul>//' > NEWS.sedit
cat NEWS.sedit
#rm sedit

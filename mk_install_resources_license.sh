#!/bin/sh
#
# Script to format the Mac OS X License.html file from 
# the standard README file.
#
dirname=`dirname $0`

cat <<NNNN>sedit_lic
1i\\
<html><pre>
1,/Adam Bernstein/{
/Adam Bernstein/a\\
<\/pre>
}
s/==*/<ul>/
s/^- /<li>/
/Please report bugs to/i\\
<\/ul>\\
<p>
s/^$/<p>/
s/^Copyrights/<p>Copyrights/
NNNN
#               Version __MAJOR__.__MINOR__ __TYPE____CKPOINT__ __MONTH__ __DAY__, __YEAR__

sed -f sedit_lic README | $dirname/mk_install_version.sh > README.sedit
cat README.sedit

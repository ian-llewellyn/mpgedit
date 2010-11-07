#!/bin/sh
#
# Script to make a "shar" like file from two components, the 
# shell script self-extraction prefix code, and the tgz suffix code.
# The output file is named the same as the input tgz file, but with a .exe
# extension.  This file is a Bourne shell script and compressed tar file data
# that the shell script will extract.  The file is named .exe to account
# for web server download issues.

argv0="make_shtgzexe.sh"
tmpfile="/tmp/make_shtgzexe$$"

if [ -z "$1" ]; then
  echo "usage: $argv0 file.tgz file"
  exit 1
fi

infile="$1"
if [ ! -f "$infile" ]; then
    echo "ERROR: Unable to read input file '$infile'"
    exit 1
fi

if [ `echo "$infile" | grep -c '\.tgz$'` -ne 1 ]; then
    echo "Error: Input file is not a tar-gz (.tgz) file"
    exit 1
fi
prefix=`basename $infile | sed 's/\.tgz$//'`
exename="${prefix}.exe"
tarname="${prefix}.tar"


# Create the Bourne shell script code
#
cat <<NNNN> $exename
#!/bin/sh
X_FLAG=NO
dir=\`echo \$0 | sed 's|\(.*/\).*|\1|'\`
base=\`echo \$0 | sed 's|.*/||'\`
if [ -n "\$dir" ]; then
  tgzname="\$dir/$exename"
else
  tgzname="$exename"
fi

if [ "x\$1" = "x-h" ]; then
  echo "usage: \$base [-h][-x][-t]"
  echo "       -h: this help"
  echo "       -x: untar the uncompressed/extracted tar file"
  echo "       -t: test the integrity of the archive by listing the contents"
  echo ""
  echo "The default operation of this script is to extract the embedded tar"
  echo "file from this script, then quit."
  exit 0
fi
if [ "x\$1" = "x-t" ]; then
  dd if=\$tgzname bs=1024 skip=1 | gzip -dc | tar tvf -
  exit \$?
fi
if [ "x\$1" = "x-x" ]; then
  X_FLAG=YES
fi
echo "Extracting ${prefix}.tar..."
dd if=\$tgzname bs=1024 skip=1 | gzip -dc > $tarname
status=\$?
if [ \$X_FLAG = YES ]; then
  tar -xvf ${prefix}.tar
fi
echo "Extraction completed."
exit \$status
NNNN

# Now write 1024 characters to a temporary file
#
i=0
while [ $i -lt 32 ]; do
  echo "###############################" >> ${tmpfile}_hash
  i=`expr $i + 1`
done

# Now determine the size of the pad needed to make the script portion of
# the file exactly 1024 bytes.
#
size=`wc -c $exename | awk  '{print $1}'`
remainder=`echo 1024 - $size | bc`
if [ $remainder -lt 0 ]; then
  echo "FATAL ERROR while computing pad after script.  This means the" 
  echo "shell script code became too large and the pad must be enlarged to"
  echo "the next 1K block."
  exit 1
fi


# Now write "remainder" bytes of comment characters onto the end of
# the shar file. 
#
dd if=${tmpfile}_hash bs=1 count=$remainder >> $exename
rm ${tmpfile}_hash

# Catenate the tgz file contents onto the end of the script and we are done.
#
cat $infile >> $exename
chmod 755 $exename

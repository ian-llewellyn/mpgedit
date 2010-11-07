#!/usr/bin/perl
#
# new descramble.pl. This approach does not chop scramble file into
# individual frames.  It figures out the position of each frame in the
# scramble file, gets their times from the scramble.mp3 -vv output,
# then builds the edit command based on that translation matrix.
# This method is far faster than the previous chop, sort, join 
# algorithm.
#

my %trans_matrix, @scramblevv, $i;

unlink("descramble.mp3");
unlink("descramble_x.mp3");

$ENV{"PATH"} = ".:" . $ENV{"PATH"};
$silent  = "-s";
$outfile = "descramble.mp3";

# Generate translation matrix.  This is where the sorting of the frames
# occurs.  This operation is far faster than creating separate files
# on disk.  Note use of associative array in this operation.  This creates
# a "database" that can be indexed by sequential frame number.
#
open(DESFP, "scramble.out") || die("opening scramble.out");
for ($i=0; <DESFP>; $i++) {
  chop;
  s/:\s.*//;
  $trans_matrix{"$_"} = $i;
}
close(DESFP);
$len=$i;

# Read in the scrambled files -vv output.  This array is then indexed
# by the translation matrix to generate the time offsets for the
# descramble edits.
# 
@scramble_out=`mpgedit -vv scramble.mp3`;
foreach (@scramble_out) {
  chop;
  (/^t=/) || next;
  s/^t=//;
  s/s\s\s*.*//;
  push(@scramblevv, "$_");
}

# Save the final frame of the file.  Since this frame may be shorter
# than the frame header says it is, there is no way to unscramble a file
# should this frame appear in the middle of the file.
#
$end1 = "$scramblevv[@scramblevv - 1]";
$end2 = "$scramblevv[@scramblevv - 2]";
print("mpgedit $silent -o descramble_x.mp3 -e$end2-$end1 scramble.mp3\n");
$output=`mpgedit -o descramble_x.mp3 -e$end2-$end1 scramble.mp3`;
print("$output");

$edits="";
$append="";
for ($i=0, $ix=1, $cnt=0; $i < $len; $i++, $ix++) {
  $indx = $trans_matrix{"$ix"};
  if ($indx == 0) {
    $edits .= " -e0.0-";
  }
  else {
     $edits .= " -e$scramblevv[$indx-1]-";
  }
  $edits .= "$scramblevv[$indx]";
  $cnt++;

  # Don't allow the command line length to grow too long.  120 edits
  # is about 3 seconds of play time.
  #
  if ($cnt >= 120) {
    $cnt = 0;
    print("mpgedit $silent -o $append$outfile $edits scramble.mp3\n");
    $output=`mpgedit $silent -o $append$outfile $edits scramble.mp3`;
    print("$output");
    $append = "+";
    $edits = "";
  }
}

# Execute the residual command line.
#
if (length("$edits") > 0) {
  print("mpgedit $silent -o $append$outfile $edits scramble.mp3\n");
  $output=`mpgedit $silent -o $append$outfile $edits scramble.mp3`;
  print("$output");
}

# Splice the last frame back onto the descrambled file.
#
print("mpgedit $silent -o +$outfile -e- descramble_x.mp3\n");
$output=`mpgedit $silent -o +$outfile -e- descramble_x.mp3`;
print("$output");

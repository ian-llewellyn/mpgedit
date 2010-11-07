#!/usr/bin/perl
use mpgtests;
use File::Compare;

$mpgedit="/home/number6/wrk/cvs/mpgedit/production/mpgedit_0.5/mpgedit_0.5_linux/mpgedit";
$mpgedit="/home/number6/wrk/cvs/mpgedit/production/mpgedit_0.6/linux/mpgedit/mpgedit";
$mpgedit="./mpgedit";


$silent ="-s";
$tn=0;
$errcnt=0;


sub edit_tail($$$)
{
  my ($infile, $outfile, $secs) = @_;
  my $output;

  $output = `$mpgedit $silent -o $outfile -e $secs- $infile`;
  print("$output");
}


sub test1_cleanup()
{
    my $i, $rmfile;

    my @rmlist = (
      "test1.idx", "test2.idx", "test1.times", "test2.times", 
    "test1_1.times", "test1_splice.mp3", 
    "test1_end_splice.mp3", "test2_splice.mp3", 
    "test3_splice.mp3", "test1_1_splice.mp3", "test1_splice31.mp3", 
    "test1_splice38.mp3", "test1_x.mp3", "test1_x.idx", 
    "test2_x.mp3", "test2_x.idx", "test1_x_1.mp3", 
    "test1_x_1.idx", "test2_x_1.mp3", "test2_x_1.idx", 
    "ATrain.idx", "ATrain23.idx", "ATrain23.times", 
    "ATrain23_splice.mp3", "ATrain23_end_splice.mp3", "ATrain_noid3.mp3");

    for (@rmlist) {
        unlink;
    }

    opendir(DIR, '.');
    for (readdir(DIR)) {
        /test1_\d\d*.mp3$/             && unlink;
        /test1_\d\d*.idx$/             && unlink;

        /test1_end_\d\d*.mp3$/          && unlink;
        /test1_end_\d\d*.idx$/          && unlink;

        /test2_\d\d*.mp3$/             && unlink;
        /test2_\d\d*.idx$/             && unlink;

        /test1_1_\d\d*.mp3$/           && unlink;
        /test1_1_\d\d*.idx$/           && unlink;

        /ATrain23_\d\d*.mp3$/          && unlink;
        /ATrain23_\d\d*.idx$/          && unlink;

        /ATrain23_end_\d\d*.mp3$/      && unlink;
        /ATrain23_end_\d\d*.idx$/      && unlink;

        /ATrain_\d\d*.mp3$/            && unlink;
        /ATrain_\d\d*.idx$/            && unlink;

        /test1_splice\d\d*.mp3$/       && unlink;
        /test1_splice\d\d*_\d\d*.mp3$/ && unlink;
    }
    test4_cleanup();
    closedir(DIR);
}


sub signal_handler {
    my $signame = shift;
    $shucks++;
    test1_cleanup();
    exit(1);
}

#
# Signal handler
#
$SIG{INT} = \&signal_handler;

sub test1_prefix_suffix($$$$$)
{
    my ($msg, $prefix, $infile, $suffix, $outfile) = @_;
    my $editfile;
#
# Test VBR encoded file with junk data at head of file is skipped, and
# ID3 tag at end of file is ignored.
#
$tn++;
print("Test $tn: $msg\n");
test1_cleanup();
append_file("$prefix", "$outfile");
append_file("$infile", "$outfile");
append_file("$suffix", "$outfile");
print("Test $tn: performing edits\n");
$output=`$mpgedit $silent -e- "$outfile"`;
print("$output");
print("Test $tn: Comparing edited file with original unmodified file\n");
($editfile = "$outfile") =~ s/\.mp3/_1.mp3/;

if (compare("$infile", "$editfile") != 0) {
  die("Failed test $tn");
}
test1_cleanup();
print("\n");
}


sub test1_1()
{
#
# Test editing entire file into output file; expensive mp3 file copy :/
#
$tn++;
print("Test $tn: Editing entire file into output file\n");
test1_cleanup();
$output=`$mpgedit $silent -e- test1.mp3`;
print("$output");
print("Test $tn: Comparing output file with original\n");
if (compare("test1_1.mp3", "test1.mp3") != 0) {
  die("Failed compare test 1");
}
test1_cleanup();
print("\n");
}



sub test1_2()
{
#
# Test cutting file into four pieces
#
$tn++;
print("Test $tn: Cutting test1.mp3 into 4 pieces\n");
test1_cleanup();
$output=`$mpgedit $silent -e-:6.500 -e:6.500-:12.500 -e:12.500-:19 -e:19- test1.mp3`;
print("$output");

print("Test $tn: Splicing 4 pieces together\n");
$cmd="$mpgedit $silent -e- -o test1_splice.mp3 test1_1.mp3" .
     " test1_2.mp3 test1_3.mp3 test1_4.mp3";
$output=`$cmd`;
print("$output");

print("Test $tn: Comparing spliced file with original\n");
if (compare("test1_splice.mp3",  "test1.mp3") !=0) {
  die("Failed split/splice test $tn");
}
test1_cleanup();
print("\n");
}


sub test1_3()
{
#
# Test cutting file into 31 1 second pieces
#
$tn++;
print("Test $tn: Cutting test1.mp3 into 31,  1 second segments\n");
test1_cleanup();
$cmd="$mpgedit $silent                      " .
  "-e0-1   -e1-2   -e2-3   -e3-4   -e4-5   " .
  "-e5-6   -e6-7   -e7-8   -e8-9   -e9-10  " .
  "-e10-11 -e11-12 -e12-13 -e13-14 -e14-15 " .
  "-e15-16 -e16-17 -e17-18 -e18-19 -e19-20 " .
  "-e20-21 -e21-22 -e22-23 -e23-24 -e24-25 " .
  "-e25-26 -e26-27 -e27-28 -e28-29 -e29-30 " .
  "-e30- test1.mp3";
`$cmd`;

print("Test $tn: Splicing 31 pieces together\n");

$cmd="$mpgedit $silent -e- -o test1_splice31.mp3                    " .
  "test1_1.mp3  test1_2.mp3  test1_3.mp3  test1_4.mp3  test1_5.mp3  " .
  "test1_6.mp3  test1_7.mp3  test1_8.mp3  test1_9.mp3  test1_10.mp3 " .
  "test1_11.mp3 test1_12.mp3 test1_13.mp3 test1_14.mp3 test1_15.mp3 " .
  "test1_16.mp3 test1_17.mp3 test1_18.mp3 test1_19.mp3 test1_20.mp3 " .
  "test1_21.mp3 test1_22.mp3 test1_23.mp3 test1_24.mp3 test1_25.mp3 " .
  "test1_26.mp3 test1_27.mp3 test1_28.mp3 test1_29.mp3 test1_30.mp3 " .
  "test1_31.mp3";
`$cmd`;

print("Test $tn: Comparing spliced file with original\n");
if (compare("test1_splice31.mp3", "test1.mp3") != 0) {
  die("Failed split/splice test $tn");
}
test1_cleanup();
print("\n");
}

sub test1_4()
{
#
# Test cutting file into 31 1 second pieces, using fractional edit boundaries
#
$tn++;
print("Test $tn: Cutting test1.mp3 into 31, 1 second segments with fractional\n");
print("        start and end time\n");
test1_cleanup();
$cmd = "$mpgedit $silent                                           " .
  "-e0-1.50        -e1.50-2.100    -e2.100-3.150   -e3.150-4.200   " .
  "-e4.200-5.250   -e5.250-6.300   -e6.300-7.350   -e7.350-8.400   " .
  "-e8.400-9.450   -e9.450-10.500  -e10.500-11.550 -e11.550-12.600 " .
  "-e12.600-13.650 -e13.650-14.700 -e14.700-15.750 -e15.750-16.800 " .
  "-e16.800-17.850 -e17.850-18.900 -e18.900-19.950 -e19.950-20.25  " .
  "-e20.25-21.75   -e21.75-22.125  -e22.125-23.175 -e23.175-24.225 " .
  "-e24.225-25.275 -e25.275-26.325 -e26.325-27.375 -e27.375-28.425 " .
  "-e28.425-29.475 -e29.475-30.525 -e30.525- test1.mp3";
`$cmd`;

print("Test $tn: Splicing 31 pieces together\n");
$cmd = "$mpgedit $silent -e- -o test1_splice31_2.mp3                " .
  "test1_1.mp3  test1_2.mp3  test1_3.mp3  test1_4.mp3  test1_5.mp3  " .
  "test1_6.mp3  test1_7.mp3  test1_8.mp3  test1_9.mp3  test1_10.mp3 " .
  "test1_11.mp3 test1_12.mp3 test1_13.mp3 test1_14.mp3 test1_15.mp3 " .
  "test1_16.mp3 test1_17.mp3 test1_18.mp3 test1_19.mp3 test1_20.mp3 " .
  "test1_21.mp3 test1_22.mp3 test1_23.mp3 test1_24.mp3 test1_25.mp3 " .
  "test1_26.mp3 test1_27.mp3 test1_28.mp3 test1_29.mp3 test1_30.mp3 " .
  "test1_31.mp3";
`$cmd`;

print("Test $tn: Comparing spliced file with original\n");
if (compare("test1_splice31_2.mp3",  "test1.mp3") != 0) {
  die("Failed split/splice test $tn");
}
test1_cleanup();
print("\n");
}


sub test1_5()
{
#
# Cut first second of test file into individual frames
#
$tn++;
print("Test $tn: Cutting test1.mp3 into 39, 26 millisecond segments\n");
test1_cleanup();
$cmd = "$mpgedit $silent " .
"-e0:0.0-0:0.26    -e0:0.26-0:0.52   -e0:0.52-0:0.78   -e0:0.78-0:0.104  " .
"-e0:0.104-0:0.130 -e0:0.130-0:0.156 -e0:0.156-0:0.182 -e0:0.182-0:0.208 " .
"-e0:0.208-0:0.234 -e0:0.234-0:0.260 -e0:0.260-0:0.286 -e0:0.286-0:0.312 " .
"-e0:0.312-0:0.338 -e0:0.338-0:0.364 -e0:0.364-0:0.390 -e0:0.390-0:0.416 " .
"-e0:0.416-0:0.442 -e0:0.442-0:0.468 -e0:0.468-0:0.494 -e0:0.494-0:0.520 " .
"-e0:0.520-0:0.546 -e0:0.546-0:0.572 -e0:0.572-0:0.598 -e0:0.598-0:0.624 " .
"-e0:0.624-0:0.650 -e0:0.650-0:0.676 -e0:0.676-0:0.702 -e0:0.702-0:0.728 " .
"-e0:0.728-0:0.754 -e0:0.754-0:0.780 -e0:0.780-0:0.806 -e0:0.806-0:0.832 " .
"-e0:0.832-0:0.858 -e0:0.858-0:0.884 -e0:0.884-0:0.910 -e0:0.910-0:0.936 " .
"-e0:0.936-0:0.962 -e0:0.962-0:0.988 -e0:0.988-0:0.1014 test1.mp3";
`$cmd`;


print("Test $tn: Cutting a 1 second segment of test1.mp3\n");
`$mpgedit $silent -e0-1 test1.mp3`;

print("Test $tn: Splicing 39 pieces together\n");
$cmd = "$mpgedit $silent -o test1_splice38.mp3                 " .
"-e- test1_1.mp3  test1_2.mp3  test1_3.mp3               " .
"    test1_4.mp3  test1_5.mp3  test1_6.mp3  test1_7.mp3  " .
"    test1_8.mp3  test1_9.mp3  test1_10.mp3 test1_11.mp3 " .
"    test1_12.mp3 test1_13.mp3 test1_14.mp3 test1_15.mp3 " .
"    test1_16.mp3 test1_17.mp3 test1_18.mp3 test1_19.mp3 " .
"    test1_20.mp3 test1_21.mp3 test1_22.mp3 test1_23.mp3 " .
"    test1_24.mp3 test1_25.mp3 test1_26.mp3 test1_27.mp3 " .
"    test1_28.mp3 test1_29.mp3 test1_30.mp3 test1_31.mp3 " .
"    test1_32.mp3 test1_33.mp3 test1_34.mp3 test1_35.mp3 " .
"    test1_36.mp3 test1_37.mp3 test1_38.mp3 test1_39.mp3";
`$cmd`;

print("Test $tn: Comparing spliced file with 1 second segment\n");
if (compare("test1_40.mp3", "test1_splice38.mp3") != 0) {
  die("Failed split/splice test $tn");
}
test1_cleanup();
print("\n");
}


sub test1_6()
{
#
# Slice and dice test
#
$tn++;
test1_cleanup();
edit_tail("test1.mp3", "test1_end.mp3", "27");
slice_and_splice("test1_end", "mp3");
unlink("test1_end.mp3");
test1_cleanup();
print("\n");
}


sub test1_7()
{
#
# Slice and dice test with source file ending on an odd millisecond boundary
#
$tn++;
test1_cleanup();
print("Test $tn: Editing a 6.555s segment from test1.mp3\n");
print("        This tests editing a file ending on an odd millisecond boundary\n");
`$mpgedit $silent -e-:6.555 test1.mp3`;
print("Test $tn: Cutting segment test1_1.mp3 into individual frames\n");
slice_and_splice("test1_1", "mp3");
test1_cleanup();
print("\n");
}

sub test1_8()
{
#
# Cut CBR encoded file with EOF not on an even frame boundary in
# two pieces, then join
$tn++;
test1_cleanup();
print("Test $tn: Editing CBR file with EOF not on an even frame boundary\n");
`$mpgedit $silent -e-3.555 -e3.555- test2.mp3`;
#
# These two command lines are the same, but the first one used to crash
# while the second worked, so test both here
#
`$mpgedit $silent -f test2_1.mp3 -f test2_2.mp3 -e-  -o test2_splice.mp3`;
`$mpgedit $silent -o test3_splice.mp3 -e- test2_1.mp3 test2_2.mp3`;
if (compare("test2.mp3", "test2_splice.mp3") != 0) {
    die("Failed test $tn - 1");
}
if (compare("test2.mp3", "test3_splice.mp3") != 0) {
    die("Failed test $tn - 2");
}
test1_cleanup();
print("\n");
}



sub test1_9()
{
#
# Slice and dice test using CBR encoded file, and EOF is not on an even
# frame boundary.
#
$tn++;
test1_cleanup();
print("Test $tn: Slice and dice test using CBR encoded file, and EOF is\n");
print("        not on an even frame boundary.\n");
slice_and_splice("test2", "mp3");
test1_cleanup();
print("\n");
}




sub test1_10()
{
#
# Test VBR encoded file with random junk at head of file is skipped, and
# ID3 tag at end of file is ignored.
#
$tn++;
print("Test $tn: Constructing VBR file, with ID3 tag and junk data prefix\n");
test1_cleanup();
append_file("ones_data", "test1_x.mp3");
append_file("ones_data", "test1_x.mp3");
append_file("ones_data", "test1_x.mp3");
append_file("ones_data", "test1_x.mp3");
append_file("ones_data", "test1_x.mp3");
append_file("ones_data", "test1_x.mp3");

append_file("test1.mp3", "test1_x.mp3");

append_file("test2.tag", "test1_x.mp3");
append_file("ones_data", "test1_x.mp3");
append_file("ones_data", "test1_x.mp3");
append_file("ones_data", "test1_x.mp3");
append_file("ones_data", "test1_x.mp3");
append_file("ones_data", "test1_x.mp3");
append_file("ones_data", "test1_x.mp3");

print("Test $tn: $mpgedit edit of entire file should remove prefix and ID3 tag\n");
`$mpgedit $silent -e- test1_x.mp3`;
print("Test $tn: Comparing edited file with original unmodified file\n");
if (compare("test1.mp3", "test1_x_1.mp3") != 0) {
  die("Failed test $tn");
}
test1_cleanup();
print("\n");
}

sub test1_11()
{
#
# Test editing of MPEG 2 Layer 3 encoded file.  Just cut entire file
# and compare against original.  Remember, must remove ID3 tag before
# comparison, as mpgedit also strips the ID3 tag.
#
if (-f "ATrain.mp3") {
  test1_cleanup();
  $tn++;
  print("Test $tn: Editing entire MPEG 2 layer 3 file\n");
  $len = (-s "ATrain.mp3") - 128;

  print("Test $tn: Stripping ID3 tag from source file\n");
  append_file("ATrain.mp3", "ATrain_noid3.mp3", $len);

  print("Test $tn: Cutting entire MPEG 2 file\n");
  `$mpgedit $silent -e- ATrain.mp3`;

  print("Test $tn: Comparing cut file with original file\n");
  if (compare("ATrain_1.mp3", "ATrain_noid3.mp3") != 0) {
    die("Failed test $tn");
  }
  test1_cleanup();
  print("\n");
}
}


sub test1_12()
{
#
# Edit entire MPEG 2 file
#
$tn++;
test1_cleanup();
print("Test $tn: Editing entire MPEG 2 layer 3 file\n");
`$mpgedit $silent -e- ATrain23.mp3`;

print("Test $tn: Comparing cut file with original file\n");
if (compare("ATrain23_1.mp3", "ATrain23.mp3") != 0) {
  die("Failed test $tn");
}
test1_cleanup();
print("\n");
}


sub test1_13()
{
#
# Edit MPEG 2 file into multiple pieces
#
$tn++;
test1_cleanup();
print("Test $tn: Cutting ATrain23.mp3 into 8 pieces\n");
$cmd = "$mpgedit $silent -e-3.33   -e3.33-6.66 -e6.66-9.99 -e9.99-12 " .
                 "-e12-15.1 -e15.1-18.5 -e18.5-21   -e21- ATrain23.mp3";
`$cmd`;

print("Test $tn: Splicing 8 pieces together\n");
$cmd = "$mpgedit $silent -o ATrain23_splice.mp3 -e-                    " .
  "ATrain23_1.mp3 ATrain23_2.mp3 ATrain23_3.mp3 ATrain23_4.mp3  " .
  "ATrain23_5.mp3 ATrain23_6.mp3 ATrain23_7.mp3 ATrain23_8.mp3";
`$cmd`;

print("Test $tn: Comparing cut file with original file\n");
if (compare("ATrain23_splice.mp3", "ATrain23.mp3") != 0) {
  die("Failed test $tn");
}
test1_cleanup();
print("\n");
}


sub test1_14()
{
#
# Chop MPEG 2 file into individual frames
#
$tn++;
print("Test $tn: Cutting entire MPEG 2 layer 3 file into individual frames\n");
test1_cleanup();
edit_tail("ATrain23.mp3", "ATrain23_end.mp3", "17");
slice_and_splice("ATrain23_end", "mp3");
($? != 0)  && exit(1);
unlink("ATrain23_end.mp3");
test1_cleanup();
print("\n");
}

sub test1_15()
{
#
# Call MPEG1 layer 1/2/3, MPEG2 layer 2/3 parsing test script
#
  $tn++;
  (test4_1("mpg1_layer1_fl1.mp3",    "384", "49", "28224")) && $errcnt++;
  (test4_1("mpg1_layer1_fl7.mp3",    "384", "63", "26332")) && $errcnt++;
  (test4_1("mpg1_layer2_fl11.mp3",   "192", "49", "30720")) && $errcnt++;
  (test4_1("mpg1_layer2_fl16.mp3",   "256", "63", "48384")) && $errcnt++;
  (test4_1("mpg2_layer2_test24.mp3", "96",  "27", "23328")) && $errcnt++;
  (test4_2("test1.mp3",              "48",  "224", "144", "1264", "596332")) && $errcnt++;
  (test4_1("ATrain23.mp3", "64",     "896", "187245")) && $errcnt++;
  print("\n");
}

sub test1_16()
{
#
# Call VBR test script.
#
  $tn++;
  (test3_main()) && $errcnt++;

}



#
# Perform scramble/descramble test of specified file
#
sub test1_17($)
{
  my ($file) = @_;
  my ($output, $rsts);

  test1_cleanup();
  scramble_file_cleanup();
  $tn++;
  print("Test $tn: Performing scramble/descramble test on '$file'\n");
  $rsts=scramble_main("$file", 0);
  ($rsts == 0) || die ("Failed test $tn: 'scramble $file' failed\n");

  $rsts = unscramble_main("scramble.mp3", 0);
  ($rsts == 0) || die ("Failed test $tn: 'unscramble scramble.mp3' failed\n");
  print("          Comparing descramble.mp3 to original '$file'\n");
  if (compare("$file",  "descramble.mp3") != 0) {
    die("Failed test $tn: '$file' and 'descramble.mp3' do not compare\n");
  }
  print("          scramble/descramble test on '$file' successful\n\n");
  unscramble_file_cleanup();
}


sub test1_18()
{
  $tn++;
  print("Test $tn: Xing header fixup (mpgedit -X1) test\n");

  unlink("test1x.mp3");
  @output=`$mpgedit test1.mp3`;
  foreach (@output) {
    if (/Total frames:/) {
      chop;
      ($test1_frames=$_) =~ s/.*\s\s*//;
    }
    elsif (/File size:/) {
      chop;
      ($test1_size=$_) =~ s/.*\s\s*//;
    }
  }
  
  @output=`$mpgedit test2.mp3`;
  foreach (@output) {
    if (/Total frames:/) {
      chop;
      ($test2_frames=$_) =~ s/.*\s\s*//;
    }
    elsif (/File size:/) {
      chop;
      ($test2_size=$_) =~ s/.*\s\s*//;
    }
  }
  
  append_file("test1.mp3", "test1x.mp3");
  append_file("test2.mp3", "test1x.mp3");
  
  @output=`$mpgedit -v test1x.mp3`;
  #print("$output[5]");
  #print("$output[6]");
  #print("$output[7]");
  #print("$output[8]");
  ($xingframes_init=$output[6])  =~ s/.*=\s\s*//;
  ($xingbytes_init =$output[7])  =~ s/.*=\s\s*//;
  chop $xingframes_init;
  chop $xingbytes_init;
  
  
  `$mpgedit -s -X1 test1x.mp3`;
  
  @output=`$mpgedit -v test1x.mp3`;
  ($xingframes_final=$output[6])  =~ s/.*=\s\s*//;
  ($xingbytes_final =$output[7])  =~ s/.*=\s\s*//;
  chop $xingframes_final;
  chop $xingbytes_final;
  
  `$mpgedit -s -X1 test1x.mp3`;
  $total_size = $test1_size + $test2_size;
  $total_frames = $test1_frames + $test2_frames;
  ($xingframes_final == $total_frames) || 
    die("Xing header frame count is incorrect; " .
        "$xingframes_final, should be $total_frames");
  
  ($xingbytes_final == $total_size) ||
    die("Xing header file size is incorrect; " .
        "$xingbytes_final, should be $total_size");
  
  print("Test $tn: SUCCESS->Xing header adjusted correctly; " .
        "size='$xingbytes_final' frames='$xingframes_final'\n\n");
}


sub unxing_file($$$)
{
  my ($infile, $outfile, $len) = @_;

  # Chop Xing header off of test1.mp3.
  #
  open(INFP19, "$infile") || die("test $tn: failed opening file '$infile'");
  binmode(INFP19);
  open(OUTFP19, ">$outfile") ||
      die("test $tn: failed opening outout file '$outfile'");
  binmode(OUTFP19);
  seek(INFP19, $len, 0) ||
      die("test $tn: failed seeking $len bytes in file '$infile'");
  while(<INFP19>) {
    print(OUTFP19);
  }
  close(INFP19);
  close(OUTFP19);

}


sub test1_19()
{

  $tn++;
  print("Test $tn: Repair VBR file with missing Xing header\n");
  unlink("fixedheader.mp3");
  unlink("noheader.mp3");

  unxing_file("test1.mp3", "noheader.mp3", 208);

  # Use mpgedit to repair missing Xing header for "noheader.mp3"
  #
  # TBD/adam: FIX xingheader.mp3.  There are now 6 templates in mpgedit, so
  # just picking what xingheaderi.mp3 is laying around is dangerous.
  # Must call test1_21() first, to force xingheader.mp3 to be built with the
  # correct template type for this test.
  #
  $output=`$mpgedit $silent -o fixedheader.mp3 -e- xingheader.mp3 noheader.mp3`;
  print $output;
  
  $test1_stats=`$mpgedit -s test1.mp3`;
  $xingtest_stats=`./mp3_vbrpatch fixedheader.mp3`;
  $sts = test_statistics("$test1_stats",
                         "$xingtest_stats",
                         "test1.mp3",
                         "");
  if ($sts > 0) {
      die("Failed test $tn: 'fixedheader.mp3' and ".
          "'test1.mp3' do not compare\n");
  }
  unlink("fixedheader.mp3");
  unlink("noheader.mp3");
  print("\n");
}


sub test1_20()
{

  $tn++;
  print("Test $tn: Add Xing header to CBR file\n");
  unlink("fixedheader.mp3");

  $output=`$mpgedit $silent -o fixedheader.mp3 -e- xingheader.mp3 test2.mp3`;
  print $output;
  $output=`./mp3_vbrpatch fixedheader.mp3`;
  ("$output" =~ /frames\s\s*=\s299/) ||
      die("Failed test $tn: frames invalid\n");
  ("$output" =~ /bytes\s\s*=\s155984/) ||
      die("Failed test $tn: bytes invalid\n");
  ("$output" =~ /vbr_scale\s\s*=\s66/) ||
      die("Failed test $tn: vbr scale invalid\n");
  unlink("fixedheader.mp3");
  print("\n");
}


sub test1_21()
{

  $tn++;
  print("Test $tn: Repair 44100 Stereo VBR " .
        "file with missing Xing header with -X2\n");
  unlink("noheader_newxing_1.mp3");
  unlink("noheader.mp3");
  unlink("xingheader.mp3");

  unxing_file("test1.mp3", "noheader.mp3", 208);

  # Use mpgedit to repair missing Xing header for "noheader.mp3"
  #
  $output=`$mpgedit $silent -X2 noheader.mp3`;
  print $output;


  $test1_stats=`$mpgedit -s test1.mp3`;
  $xingtest_stats=`./mp3_vbrpatch noheader_newxing_1.mp3`;
  $sts = test_statistics("$test1_stats",
                         "$xingtest_stats",
                         "test1.mp3",
                         "noheader");
  if ($sts > 0) {
      die("Failed test $tn: 'fixedheader.mp3' and ".
          "'test1.mp3' do not compare\n");
  }

  unlink("noheader_newxing_1.mp3");
  unlink("noheader.mp3");
  print("\n");
}


sub test1_22($$$)
{
  my ($file, $xing_size, $msg) = @_;

  $tn++;
  print("Test $tn: Repair $msg VBR file with missing Xing header with -X2\n");
  unlink("noheader_newxing_1.mp3");
  unlink("noheader.mp3");
  unlink("xingheader.mp3");

  unxing_file($file, "noheader.mp3", $xing_size);

  # Use mpgedit to repair missing Xing header for "noheader.mp3"
  #
  $output=`$mpgedit $silent -X2 noheader.mp3`;
  print $output;


  $test1_stats=`$mpgedit -s $file`;
  $xingtest_stats=`./mp3_vbrpatch noheader_newxing_1.mp3`;
  $sts = test_statistics("$test1_stats",
                         "$xingtest_stats",
                         "$file",
                         "noheader");
  if ($sts > 0) {
      die("Failed test $tn: 'noheader_newxing_1.mp3' and ".
          "'$file' do not compare\n");
  }

  unlink("noheader_newxing_1.mp3");
  unlink("noheader.mp3");
  print("\n");
}



sub test1_23($)
{
  my ($file) = @_;
  my @vvlist;
  my $l1 = "t=0.026s  br=224  sz=732  fr=1      pos=208        pos=0xd0";
  my $l2 = "t=0.052s  br=112  sz=366  fr=2      pos=940        pos=0x3ac";
  my $l3 = "t=0.078s  br=80   sz=262  fr=3      pos=1306       pos=0x51a";
  my $l4 = "t=0.104s  br=48   sz=157  fr=4      pos=1568       pos=0x620";
  my $l5 = "t=0.130s  br=64   sz=209  fr=5      pos=1725       pos=0x6bd";
  my $l6 = "t=0.156s  br=80   sz=262  fr=6      pos=1934       pos=0x78e";
  my $l7 = "t=0.182s  br=96   sz=314  fr=7      pos=2196       pos=0x894";
  my $l8 = "t=0.208s  br=112  sz=366  fr=8      pos=2510       pos=0x9ce";
  my $l9 = "t=0.235s  br=224  sz=732  fr=9      pos=2876       pos=0xb3c";
  my $l10 = "t=0.261s  br=224  sz=732  fr=10     pos=3608       pos=0xe18";
  my $lineno = 1;

  $tn++;
  print("Test $tn: Verify -vv output for accuracy\n");
  @vvlist = `$mpgedit -vv $file`;


  ($vvlist[9] =~ /$l1/) || die("Failed verifying line $lineno");
  $lineno++;
  ($vvlist[10] =~ /$l2/) || die("Failed verifying line $lineno");
  $lineno++;
  ($vvlist[11] =~ /$l3/) || die("Failed verifying line $lineno");
  $lineno++;
  ($vvlist[12] =~ /$l4/) || die("Failed verifying line $lineno");
  $lineno++;
  ($vvlist[13] =~ /$l5/) || die("Failed verifying line $lineno");
  $lineno++;
  ($vvlist[14] =~ /$l6/) || die("Failed verifying line $lineno");
  $lineno++;
  ($vvlist[15] =~ /$l7/) || die("Failed verifying line $lineno");
  $lineno++;
  ($vvlist[16] =~ /$l8/) || die("Failed verifying line $lineno");
  $lineno++;
  ($vvlist[17] =~ /$l9/) || die("Failed verifying line $lineno");
  $lineno++;
  ($vvlist[18] =~ /$l10/) || die("Failed verifying line $lineno");
  $lineno++;
  if ($lineno == 11) {
    print("Test $tn: SUCCESS\n");
  }
}


sub main()
{
    my $all_tests = 0;
    my $cleanup = 0;

    if (length($ARGV[0]) > 0) {
        $_ = $ARGV[0];
        if (/^-a$/) {
            $all_tests = 1;
        }
        elsif (/^-v$/) {
            $silent="";
        }
        elsif (/^-c$/) {
            $cleanup=1;
        }
    }
    if ($cleanup == 1) {
        print("Only cleaning up after previous test run; no tests will be run\n");
        test1_cleanup();
        test3_cleanup();
        exit(0);
    }

    test1_1();
    test1_2();
    test1_3();
    test1_4();
    test1_5();
    test1_21();
    test1_19();
    test1_20();
    test1_6();
    test1_7();
    test1_8();
    test1_9();

    test1_prefix_suffix(
              "Constructing CBR file, with ID3 tag and prefixed with zeros",
              "zero_prefix",
              "test2.mp3",
              "test2.tag",
              "test2_x.mp3");


    test1_prefix_suffix(
              "Constructing VBR file, with ID3 tag and prefixed with zeros",
              "zero_prefix",
              "test1.mp3",
              "test2.tag",
              "test1_x.mp3");

    test1_prefix_suffix(
              "Constructing CBR file, with ID3 tag and random data prefix",
              "random_prefix", 
              "test2.mp3", 
              "test2.tag", 
              "test2_x.mp3");

    test1_prefix_suffix(
              "Constructing CBR file, with ID3 tag and junk data prefix",
              "ones_data",
              "test2.mp3",
              "test2.tag",
              "test2_x.mp3");


    test1_prefix_suffix(
             "Constructing VBR file, with ID3 tag and random data prefix",
             "random_prefix",
             "test1.mp3",
             "test2.tag",
             "test1_x.mp3");

    test1_prefix_suffix(
             "Constructing VBR file, with ID3 tag and junk data prefix",
             "ones_data",
             "test1.mp3",
             "test2.tag",
             "test1_x.mp3");
    test1_10();
    test1_11();
    test1_12();
    test1_13();
    test1_14();
    edit_tail("test1.mp3",    "test1_end.mp3", "27");
    edit_tail("ATrain23.mp3", "ATrain23_end.mp3", "17");
    test1_17("test1_end.mp3");
    test1_17("test2.mp3");
    test1_17("ATrain23_end.mp3");
    test1_17("mpg1_layer1_fl1.mp3");
    test1_17("mpg1_layer1_fl7.mp3");
    test1_17("mpg1_layer2_fl11.mp3");
    test1_17("mpg1_layer2_fl16.mp3");
    test1_17("mpg2_layer2_test24.mp3");
    unlink("test1_end.mp3");
    unlink("ATrain23_end.mp3");
    test1_18();
    test1_15();
    test1_16();

    # Xing header tests.
    test1_21();
    test1_22("test1.mp3",        208, "44100 Stereo");
    test1_22("test1_44100m.mp3", 417, "44100 Mono");
    test1_22("test1_22050s.mp3", 208, "22050 Stereo");
    test1_22("test1_22050m.mp3", 208, "22050 Mono");
    test1_22("test1_11025s.mp3", 208, "11025 Stereo");
    test1_22("test1_11025m.mp3", 208, "11025 Mono");
    test1_23("test1.mp3");

    if ($errcnt == 0) {
      print("SUCCESS: All tests passed successfully\n");
    }
    else {
      print("ERROR: '$errcnt' tests failed\n");
    }
}


main();

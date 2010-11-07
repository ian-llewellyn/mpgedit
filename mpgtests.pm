#!/usr/local/bin/perl


sub slice_and_splice($$@)
{
    my ($base, $ext, $skip_rejoin) = @_;
    my @output;

    my $t, $c, $mod, $etimes, $i;

    #
    # Execute mpgedit -vv, extracting t= value for each frame from output
    #
    open(FP, ">${base}.times") || die("opening $base.times failed");
    @output = `$mpgedit -vv "$base.$ext"`;
    foreach (@output) {
        if (/^t=/) {
            ($value = $_)  =~ s/^t=(\d\d*\.\d\d*)s.*/$1/;
            print(FP "$value");
        }
        elsif (/Total frames:/) {
            ($total_frames = $_) =~ s/Total frames:\s\s*//;
            chop $total_frames;
        }
    }
    close(FP);
    print("Test $tn: Cutting entire ${base}.$ext file into individual frames,\n");
    print("        then splicing them together again.\n");
    print("        This test takes a long, long time...\n");
    print("\n");
    print("        $total_frames edits are performed by this test\n");
    print("        working.");


    $t = 0;
    $c = 0;
    $mod = 0;
    $etimes = "";
    
    # Turn off buffered stdout
    $| = 1;
    open(FP, "${base}.times") || die("opening $base.times for reading failed");
    while (<FP>) {
        chop;
        $i = $_;
        $etimes = "$etimes -e$t-$i";
        $mod++;
        if ($mod == 38) {
            $mod = 0;
            print(".");
#            print("$mpgedit $silent $etimes $base.$ext\n");
            `$mpgedit $silent $etimes $base.$ext`;
            $etimes = "";
            $mod = 0;
        }
        $t=$i;
        $c++;
    }
    close(FP);
    if ($mod > 0) {
        print(".");
        `$mpgedit $silent $etimes $base.$ext`;
    }
    print("\n");
    if ($c != $total_frames) {
      die("Error: total number of frames edited '$c'; expected '$total_frames'\n");
    

    }

    if ($skip_rejoin) {
        return;
    }

    print("Test $tn: Rejoining $total_frames single frame edits\n");
    print("        working.");

    $c = 1;
    $mod = 0;
    $files = "";
    while ( -f "${base}_${c}.${ext}") {
        $files = "$files ${base}_${c}.${ext}";
        $mod++;
        if ($mod == 38) {
            print(".");
#            print("<$mpgedit $silent -e- -o +${base}_splice.${ext} $files>\n");
            `$mpgedit $silent -e- -o +${base}_splice.${ext} $files`;
            $files = "";
            $mod = 0;
        }
        $c++;
    }
    if ($mod > 0) {
        print(".");
        `$mpgedit $silent -e- -o +${base}_splice.${ext} $files`;
    }
    print("\n");

    print("Test $tn: Comparing spliced file with original file\n");
    if (compare("${base}_splice.$ext", "${base}.$ext") != 0) {
        die("Failed split/splice test $tn");
    }

}



#
# Copy contents of "from" file to "to" file, but append when "to" exists.
# Implements "cat from >> to".  An optional third parameter allows the
# desired copy size. The entire file is copied when this parameter is
# missing.
#
sub append_file($$@)
{
    my ($from, $to, $size) = @_;
    my $buf, $len, $filesize;

    open(_APPENDF_A_FP, ">>$to") ||
        die("append_file: failed opening '$to'");
    open(_APPENDF_R_FP, "<$from") ||
        die("append_file: failed opening '$from'");
    binmode(_APPENDF_A_FP);
    binmode(_APPENDF_R_FP);

    $filesize = (-s "$from");
    if (length($size) ==  0) {
        $size = $filesize;
    }
    elsif ($size > $filesize || $size < 0) {
        $size = $filesize;
    }

    $len = 1024;
    while ($size && ($len=read(_APPENDF_R_FP, $buf, $len))) {
        print _APPENDF_A_FP $buf;
        $size -= $len;
        $len = $size > 1024 ? 1024 : $size;
    }

    close(_APPENDF_A_FP);
    close(_APPENDF_R_FP);
}


sub test4_cleanup()
{
  my @rmlist = ("ATrain23.idx", "mpg1_layer1_fl7.idx", 
                "mpg1_layer2_fl16.idx", "test1.idx", 
                "mpg1_layer1_fl1.idx", "mpg1_layer2_fl11.idx", 
                "mpg2_layer2_test24.idx");

  for (@rmlist) {
    unlink
  }
}


sub test4_1($$$$)
{
  #
  # These are the correct answers for mpg1_layer1_fl1.mp3
  #
  # File name:    mpg1_layer1_fl1.mp3
  # CBR:          384
  # Total frames: 49
  # File size:    28224
  # Track length: 0:00.588 (0s)
  #
  
  my ($file, $cbr, $frames, $size) = @_;

  my $err=0;
  my @out;
  @out=`$mpgedit $file`;

  print("Test $tn: Parsing MPEG1 layer1 file\n");
  print("Test $tn: testing $file\n");
  for (@out) {
    chop;
    if (/CBR:/) {
      s/CBR:\s\s*//;
      if ($_ != $cbr) {
        print("Test $tn: $file: bitrate incorrect '$_'\n");
        $err = 1;
      }
    }
    elsif (/Total frames:/) {
      s/Total frames:\s\s*//;
      if ($_ != $frames) {
        print("Test $tn: $file: frame count incorrect '$_'\n");
         $err = 1;
      }
    }
    elsif (/File size:/) {
      s/File size:\s\s*//;
      if ($_ != $size) {
        print("Test $tn: $file: file size incorrect '$_'\n");
        $err=1;
      }
    }
  }
  print("\n");
  return $err;
}


sub test4_2($$$$$$)
{
  my ($file, $vbrmin, $vbrmax, $vbravg, $frames, $size) = @_;

  my $err = 0;
  my @out;
  @out=`$mpgedit $file`;

  print("Test $tn: Parsing MPEG1 layer1 file\n");
  print("Test $tn: testing $file\n");
  for (@out) {
    chop;
    if (/VBR Min:/) {
      s/VBR Min:\s\s*//;
      if ($_ != $vbrmin) {
        print("Test $tn: $file: VBR min incorrect '$_'\n");
        $err = 1;
      }
    }
    elsif (/VBR Max:/) {
      s/VBR Max:\s\s*//;
      if ($_ != $vbrmax) {
        print("Test $tn: $file: VBR max incorrect '$_'\n");
        $err = 1;
      }
    }
    elsif (/VBR Average:/) {
      s/VBR Average:\s\s*//;
      if ($_ != $vbravg) {
        print("Test $tn: $file: VBR average incorrect '$_'\n");
        $err = 1;
      }
    }
    elsif (/Total frames:/) {
      s/Total frames:\s\s*//;
      if ($_ != $frames) {
        print("Test $tn: $file: frame count incorrect '$_'\n");
        $err = 1;
      }
    }
    elsif (/File size:/) {
      s/File size:\s\s*//;
      if ($_ != $size) {
        print("Test $tn: $file: file size incorrect '$_'\n");
        $err=1;
      }
    }
  }
  print("\n");
  return $err;
}



sub test3_initialize()
{
  my $rsts = 0;

  if(length($mpgedit) == 0) {
    $mpgedit = "./mpgedit";
  }

  $_=`lame --version 2>&1`;
  if ($? != 0) {
    print("Test $tn: 'lame' must be installed to proceed\n");
    $rsts = 1;
  }

  $_=`sox --version 2>&1`;
  if ($? != 0) {
    print("Test $tn: 'sox' must be installed to proceed\n");
    $rsts = 1;
  }

  return $rsts;
}


sub test3_cleanup()
{
    opendir(DIR, '.');
    for (readdir(DIR)) {
      /test1_.*.mp3$/ && unlink("$_");
      /test1_.*.idx$/ && unlink("$_");
      /test1_.*.wav$/ && unlink("$_");
    }
    closedir(DIR);
    unlink("test1.wav");
    unlink("test1.idx");
}


sub test_statistics($$$$)
{
  my ($mpgedit, $xing, $name, $prefix) = @_;
  $noheader = "${prefix}.mp3";
  $noheader_xing = "${prefix}_newxing_1.mp3";
  $return_test_statistics=0;
  print("\n");
  print("Test $tn: Testing Xing header for '$name'\n");

  for (split(/\n/, "$mpgedit")) {
    (/^Total frames:/) && ($mpgedit_frames = "$_") =~ s/^Total frames: *//;
    (/^File size:/)    && ($mpgedit_bytes = "$_")  =~ s/File size: *//;
  }
  for (split(/\n/, "$xing")) {
    (/^frames *=/)     && ($xing_frames = "$_")    =~ s/frames *= *//;
    (/^bytes *= */)    && ($xing_bytes = "$_")     =~ s/bytes *= *//;
  }


  if ($mpgedit_frames != $xing_frames) {
    print("Test $tn: ERROR: $name frames not the same ($mpgedit_frames:$xing_frames)\n");
    $return_test_statistics=1;
  }
  elsif ($mpgedit_bytes != $xing_bytes) {
    # This is a hack to take into account the difference in sizes between
    # the Xing header that was stripped and the Xing header that replaced
    # it.  This adjustment in the test is needed here because a strict file
    # size comparison check does not work anymore.  Since the header is no
    # longer identical in size a strict file comparison will no longer work
    # either.  The number of frames will be the same, and the file size
    # will be the same, according to the relationship computed below.
    #
    $xingsize = (-s "$name") - (-s "$noheader");
    $newxingsize = (-s "xingheader.mp3");
    $xingdiff = $xingsize - $newxingsize;
    $cmp_xing_bytes = $xing_bytes + $xingdiff;
#print("xingsize=$xingsize\n");
#print("newxingsize=$newxingsize\n");
#print("xingdiff=$xingdiff\n");
#print("cmp_xing_bytes=$cmp_xing_bytes\n");
    print("         !!!! Extended size test performed !!!!\n");
    if ($mpgedit_bytes != $cmp_xing_bytes) {
      print("Test $tn: ERROR: $name bytes not the same ($mpgedit_bytes:$xing_bytes)\n");
      $return_test_statistics=1;
    }
  }
  else {
    print("Test $tn: mpgedit: fr=$mpgedit_frames bytes=$mpgedit_bytes\n");
    print("Test $tn: vbr:     fr=$xing_frames bytes=$xing_bytes\n");
  }
  return($return_test_statistics);
}


sub test3_main()
{
  my $errors;

  if (test3_initialize()) {
    print("Unable to run this test until the above components are installed\n");
    return(1);
  }

  print <<NNNN;

Test $tn:
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

NNNN
  # Decode test1.mp3 to a wave file.
  print("Test $tn: Decoding test1.mp3 -> test1.wav\n\n");
  (-f "test1.wav") || `lame -S --decode test1.mp3 test1.wav`;
  (! -f "test1.wav") && print("lame failed to decode test1.mp3\n") && return(1);


  # Lowering sample rate to create MPEG2 and MPEG2.5 files
  print("Test $tn: Converting test1.wav to 44100Hz mono...\n");
  ( -f "test1_44100m.wav" ) || `sox test1.wav          -c 1 test1_44100m.wav`;
  print("Test $tn: Converting test1.wav to 22050Hz stereo...\n");
  ( -f "test1_22050s.wav" ) || `sox test1.wav -r 22050      test1_22050s.wav`;
  print("Test $tn: Converting test1.wav to 11025Hz stereo...\n");
  ( -f "test1_11025s.wav" ) || `sox test1.wav -r 11025      test1_11025s.wav`;
  print("Test $tn: Converting test1.wav to 22050Hz mono...\n");
  ( -f "test1_22050m.wav" ) || `sox test1.wav -r 22050 -c 1 test1_22050m.wav`;
  print("Test $tn: Converting test1.wav to 11025Hz mono...\n");
  ( -f "test1_11025m.wav" ) || `sox test1.wav -r 11025 -c 1 test1_11025m.wav`;

  # Create MPEG2 encodings of this test file
  print("\nTest $tn: MPEG1 Layer3 VBR encoding 44100Hz mono wav file...\n\n");
  (-f "test1_44100m.mp3") || `lame -V 1 -S test1_44100m.wav test1_44100m.mp3`;

  print("\Test $tn: nMPEG2 Layer3 VBR encoding 22050Hz stereo wav file...\n\n");
  (-f "test1_22050s.mp3") || `lame -V 1 -S test1_22050s.wav test1_22050s.mp3`;

  print("\nTest $tn: MPEG2.5 Layer 3 VBR encoding 11025Hz stereo wav file...\n\n");
  (-f "test1_11025s.mp3") || `lame -V 1 -S test1_11025s.wav test1_11025s.mp3`;

  print("\nTest $tn: MPEG2 Layer 3 VBR encoding 22050Hz mono wav file...\n\n");
  (-f "test1_22050m.mp3") || `lame -V 1 -S test1_22050m.wav test1_22050m.mp3`;

  print("\nTest $tn: MPEG2.5 Layer 3 VBR encoding 11025Hz mono wav file...\n\n");
  (-f "test1_11025m.mp3") || `lame -V 1 -S test1_11025m.wav test1_11025m.mp3`;

  (-f "test1_44100m.mp3") || print("test1_44100m.mp3 does not exist\n") && exit(1);
  (-f "test1_22050s.mp3") || print("test1_22050s.mp3 does not exist\n") && exit(1);
  (-f "test1_11025s.mp3") || print("test1_11025s.mp3 does not exist\n") && exit(1);
  (-f "test1_22050m.mp3") || print("test1_22050m.mp3 does not exist\n") && exit(1);
  (-f "test1_11025m.mp3") || print("test1_11025m.mp3 does not exist\n") && exit(1);

  # Perform edits on MPEG2/2.5 files
  print("\nTest $tn: Editing 6 seconds segments from MPEG2/2.5 files\n");
  (-f "test1_44100m_1.mp3") || `$mpgedit -s -e:6.500-:12.500 test1_44100m.mp3`;
  (-f "test1_22050s_1.mp3") || `$mpgedit -s -e:6.500-:12.500 test1_22050s.mp3`;
  (-f "test1_11025s_1.mp3") || `$mpgedit -s -e:6.500-:12.500 test1_11025s.mp3`;
  (-f "test1_22050m_1.mp3") || `$mpgedit -s -e:6.500-:12.500 test1_22050m.mp3`;
  (-f "test1_11025m_1.mp3") || `$mpgedit -s -e:6.500-:12.500 test1_11025m.mp3`;
  (-f "test1_44100s_1.mp3") ||
              `$mpgedit -s -e:6.500-:12.500 -o test1_44100s_1.mp3 test1.mp3`;

  print("\nTest $tn: Running 'mpgedit -s' to gather file statistics\n");
  # Get mpgedit frame statistics
  $test1_44100s=`$mpgedit -s test1.mp3`;
  $test1_44100m=`$mpgedit -s test1_44100m.mp3`;
  $test1_22050s=`$mpgedit -s test1_22050s.mp3`;
  $test1_11025s=`$mpgedit -s test1_11025s.mp3`;
  $test1_22050m=`$mpgedit -s test1_22050m.mp3`;
  $test1_11025m=`$mpgedit -s test1_11025m.mp3`;

  $test1_44100s_1=`$mpgedit -s test1_44100s_1.mp3`;
  $test1_44100m_1=`$mpgedit -s test1_44100m_1.mp3`;
  $test1_22050s_1=`$mpgedit -s test1_22050s_1.mp3`;
  $test1_11025s_1=`$mpgedit -s test1_11025s_1.mp3`;
  $test1_22050m_1=`$mpgedit -s test1_22050m_1.mp3`;
  $test1_11025m_1=`$mpgedit -s test1_11025m_1.mp3`;

  # Read VBR headers with mp3_vbrpatch
  print("Test $tn: Reading file statistics from Xing header\n");
  $xingtest1_44100s=`./mp3_vbrpatch test1.mp3`;
  $xingtest1_44100m=`./mp3_vbrpatch test1_44100m.mp3`;
  $xingtest1_22050s=`./mp3_vbrpatch test1_22050s.mp3`;
  $xingtest1_11025s=`./mp3_vbrpatch test1_11025s.mp3`;
  $xingtest1_22050m=`./mp3_vbrpatch test1_22050m.mp3`;
  $xingtest1_11025m=`./mp3_vbrpatch test1_11025m.mp3`;

  $xingtest1_44100s_1=`./mp3_vbrpatch test1_44100s_1.mp3`;
  $xingtest1_44100m_1=`./mp3_vbrpatch test1_44100m_1.mp3`;
  $xingtest1_22050s_1=`./mp3_vbrpatch test1_22050s_1.mp3`;
  $xingtest1_11025s_1=`./mp3_vbrpatch test1_11025s_1.mp3`;
  $xingtest1_22050m_1=`./mp3_vbrpatch test1_22050m_1.mp3`;
  $xingtest1_11025m_1=`./mp3_vbrpatch test1_11025m_1.mp3`;

  # Test unedited file Xing headers for accuracy
  print("\n\nTest $tn: Verifying Xing header data for accuracy\n\n");
  $errors = 0;
  $sts = test_statistics("$test1_44100s", "$xingtest1_44100s", "test1.mp3", "");
  $errors += $sts;

  $sts = test_statistics("$test1_44100m",
                         "$xingtest1_44100m",
                         "test1_44100m.mp3",
                         "");
  $errors += $sts;

  $sts = test_statistics("$test1_22050s",
                         "$xingtest1_22050s",
                         "test1_22050s.mp3",
                         "");
  $errors += $sts;

  $sts = test_statistics("$test1_11025s",
                         "$xingtest1_11025s",
                         "test1_11025s.mp3",
                         "");
  $errors += $sts;

  $sts = test_statistics("$test1_22050m",
                         "$xingtest1_22050m",
                         "test1_22050m.mp3",
                         "");
  $errors += $sts;

  $sts = test_statistics("$test1_11025m",
                         "$xingtest1_11025m",
                         "test1_11025m.mp3",
                         "");
  $errors += $sts;

  # Test edited file Xing headers for accuracy
  $sts = test_statistics("$test1_44100s_1",
                         "$xingtest1_44100s_1",
                         "test1_44100s_1.mp3",
                         "");
  $errors += $sts;

  $sts = test_statistics("$test1_44100m_1",
                         "$xingtest1_44100m_1",
                         "test1_44100m_1.mp3",
                         "");
  $errors += $sts;

  $sts = test_statistics("$test1_22050s_1",
                         "$xingtest1_22050s_1",
                         "test1_22050s_1.mp3",
                         "");
  $errors += $sts;

  $sts = test_statistics("$test1_11025s_1",
                         "$xingtest1_11025s_1",
                         "test1_11025s_1.mp3",
                         "");
  $errors += $sts;

  $sts = test_statistics("$test1_22050m_1",
                         "$xingtest1_22050m_1",
                         "test1_22050m_1.mp3",
                         "");
  $errors += $sts;

  $sts = test_statistics("$test1_11025m_1",
                         "$xingtest1_11025m_1",
                         "test1_11025m_1.mp3",
                         "");
  $errors += $sts;

  unlink("test1_44100s_1.mp3");
  unlink("test1_44100m_1.mp3");
  unlink("test1_22050s_1.mp3");
  unlink("test1_11025s_1.mp3");
  unlink("test1_22050m_1.mp3");
  unlink("test1_11025m_1.mp3");

  if ($errors > 0) {
    print("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    print("Test $tn: ERROR: $errors tests failed\n");
    print("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
  }
  else {
    print("\nTest $tn: SUCCESS: All tests passed\n");
  }
  return $errors;
}


sub scramble_file_cleanup()
{
    opendir(DIR, '.');
    for (readdir(DIR)) {
      /^scramble.mp3$/   && unlink("$_");
      /^scramble.out$/   && unlink("$_");
      /^scramble_*.mp3$/ && unlink("$_");
      /^scramble_*.idx$/ && unlink("$_");
    }
    closedir(DIR);
}

#
# This test script chops up an input mp3 file into individual frames, and
# stores them in random order in a file called "scramble.mp3".  This
# scrambled file is not playable, as all of the frames are stored in 
# a garbled order.  However, the file can be unscrambled using the
# test script 'unscramble.pl'.  This script takes the scramble.mp3 and
# reorders the frames into the correct sequence, using the information
# saved in scramble.out.  scramble.out is a file generated by scramble.pl,
# which associates the frame number with the time offset for that frame.
# scramble.pl and unscramble.pl constitute reciprocal operations.
# This this sequence of commands will result in a descrambled output file
# that is identical to the original input file:
#
#   ./scramble.pl test1.mp3       -> Generates scramble file scramble.mp3
#   ./descramble.pl scramble.mp3  -> Generates descramble.mp3 from scramble.mp3
#   cmp descramble.mp3 test1.mp3  -> Prove descramble.mp3 is same as testl.mp3
#
# These two scripts constitute a real torture test for mpgedit.

# Notes:
# The external program 'scramble' generates the scramble translation
# file, called scramble.out.  scramble currently is implemented in C.
# This should be rewritten in Perl at some point.  The current C
# implementation uses popen() which is not portabile to Win32 platforms.
#
# The last frame of the input file must be specially treated.  This is
# because the actual frame length is usually shorter than the frame header
# indicates.  Should this last frame end up somewhere in the middle of the
# scramble file, this will completely break the decoding of the file.
# This is because this frame is actually shorter than the frame header
# indicates, so there is no way to tell where the actual frame ends.
# These test programs solve this problem by storing the last frame in a
# special end file, scrambles the remaining frames, then pastes the end
# frame onto the end of the scramble file.  The descramble script
# handles the final frame in a similar manner.


sub scramble_main($$)
{
  my ($file, $verbose) = @_;
  my $file_x, $file_end, @timesvv = (), $line2, $line1, $mod;
  my $cmd, $output, $append, $line;

  # Turn off buffered stdout
  $| = 1;

  # Set current directory in search path, so can find scramble_times.pl.
  # ./ prefix does not work for Win32 Perl, don't know why.
  $ENV{"PATH"} = ".:" . $ENV{"PATH"};

  # file_x is the entire file, less the last frame of the file.
  # file_end is the last frame of the file.
  #
  # The strategy here is to remove the last frame of the input file,
  # scramble the remaining frames, then paste the last frame back onto
  # the end of the file.  This special treatment of the last frame
  # is needed, because the frame header size does not reflect the true
  # byte count for the last frame. The last frame is likely to be shorter
  # than what the frame header states.  This will completely screw up the
  # descramble operation should this last frame now appear somewhere in the
  # middle of the file.
  #
  ($file_x   = $file) =~ s/(.*)(\.mp3$)/$1_x$2/;
  ($file_end = $file) =~ s/(.*)(\.mp3$)/$1_end$2/;
  unlink("$file_x");
  unlink("$file_end");
  unlink("scramble.mp3");
  unlink("scramble.out");
  @timesvv = `$mpgedit -vv $file`;
  foreach (@timesvv) {
    chop;
    if (/^t=/) {
      s/^t=//;
      s/s .*//;
      $line2 = $line1;
      $line1 = $_;
    }
  }
  $output=`$mpgedit -o $file_x   -e-$line2 $file`;
  $output=`$mpgedit -o $file_end -e$line2- $file`;
  `scramble_times.pl $file_x scramble.out`;
  
  # Scramble the input mp3 file, using the time offsets from scramble.out
  # 
  $mod=0;
  $cmd = $append = "";
  open(FP, "scramble.out") || die("failed opening scramble.out");
  while_loop:
  print("Scrambling frame order in file '$file'\n");
  print("          Working");
  while (<FP>) {
    chop;
    /-0\.000$/ && next while_loop;
    if ($mod == 0) {
      if (length($cmd) > 0) {
        $cmd .= " $file_x";
        ($verbose == 0) && print(".");
        ($verbose == 0) || print("executing command $cmd\n");
        $output=`$cmd`;
#       print("debug cmd=$cmd\n");
#       print("$output");
      }
      $cmd="$mpgedit -o ${append}scramble.mp3 ";
    }
    ($line = $_) =~ s/.* //;
    if (length($line) > 0) {
      $cmd .= " -e$line";
      $mod = ($mod + 1) % 40;
    }
    $append="+";
  }
  close(FP);

  if ($mod > 0) {
    $cmd .= " $file_x";
    ($verbose == 0) && print(".");
    ($verbose==0) || print("$cmd\n");
    $output=`$cmd`;
#print("<debug: $cmd>\n");
#print("$output");
  }
  print("\n");

  # Paste the last frame onto the output scramble file
  #
  $output=`$mpgedit -o +scramble.mp3 -e- $file_end`;


  # Tidy up
  #
  unlink("$file_x");
  unlink("$file_end");
  ($file_xidx   = $file_x)   =~ s/\.mp3$/.idx/;
  ($file_endidx = $file_end) =~ s/\.mp3$/.idx/;
  unlink("$file_xidx");
  unlink("$file_endidx");
  return 0;
}

sub unscramble_file_cleanup()
{
    opendir(DIR, '.');
    for (readdir(DIR)) {
      /^descramble_\d\d*.mp3$/  && unlink($_);
      /^descramble_\d\d*.idx$/  && unlink($_);
    }
    unlink("descramble.mp3");
    unlink("descramble_x.mp3");
    unlink("scramble.mp3");
    unlink("scramble.out");
    unlink("scramble.idx");

    closedir(DIR);
}


#
# new descramble.pl. This approach does not chop scramble file into
# individual frames.  It figures out the position of each frame in the
# scramble file, gets their times from the scramble.mp3 -vv output,
# then builds the edit command based on that translation matrix.
# This method is far faster than the previous chop, sort, join 
# algorithm.
#
sub unscramble_main($$)
{
  my ($edit_file, $verbose) = @_;
  my @scramblevv = ();
  my %trans_matrix, $i, $len, $end1, $end2, $silent;
  my $edits, $appends, $ix, $indx, $cnt, $append;

  (-f "$edit_file") ||
      die("unscramble_main: specified input file " .
          "does not exist $edit_file\n");

  unlink("descramble.mp3");
  unlink("descramble_x.mp3");
  unlink("descramble.idx");
  unlink("descramble_x.idx");

  $ENV{"PATH"} = ".:" . $ENV{"PATH"};
  $silent  = "-s";
  $outfile = "descramble.mp3";

  print("Test $tn: descrambling file $edit_file\n");
  print("          Generating translation matrix\n");
  
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
  @scramble_out=`mpgedit -vv $edit_file`;
  foreach (@scramble_out) {
    chop;
    (/^t=/) || next;
    s/^t=//;
    s/s\s\s*.*//;
    push(@scramblevv, "$_");
  }
  
  print("          Splitting final frame from scramble file\n");
  # Save the final frame of the file.  Since this frame may be shorter
  # than the frame header says it is, there is no way to unscramble a file
  # should this frame appear in the middle of the file.
  #
  $end1 = "$scramblevv[@scramblevv - 1]";
  $end2 = "$scramblevv[@scramblevv - 2]";
  ($verbose==1) &&
    print("mpgedit $silent -o descramble_x.mp3 -e$end2-$end1 $edit_file\n");
  $output=`mpgedit $silent -o descramble_x.mp3 -e$end2-$end1 $edit_file`;
  ($verbose==1) && print("$output");
  
  $edits="";
  $append="";
  $| = 1;
  print("          Performing descramble operations.\n");
  print("          Working");
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
  
    # Don't allow the command line length to grow too long.  40 edits
    # is about 1 second of play time.
    #
    if ($cnt >= 40) {
      print(".");
      if (($i + 1) >= $len) {
        $edits .= " -e- descramble_x.mp3";
      }
      $cnt = 0;
      ($verbose==1) &&
        print("mpgedit $silent -o $append$outfile $edits $edit_file\n");
      $output=`mpgedit $silent -o $append$outfile $edits $edit_file`;
      ($verbose==1) && print("$output");
      $append = "+";
      $edits = "";
    }
  }
  $| = 0;
  
  # Execute the residual command line.
  #
  if (length("$edits") > 0) {
    $edits .= " -e$end2-$end1 ";
    ($verbose==1) &&
      print("mpgedit $silent -o $append$outfile $edits $edit_file\n");
    $output=`mpgedit $silent -o $append$outfile $edits $edit_file`;
    ($verbose==1) && print("$output");
  }
  
  # Splice the last frame back onto the descrambled file.
  #
#  print("\n          Splicing final frame onto descramble file\n");
#  ($verbose==1) && print("mpgedit $silent -o +$outfile -e- descramble_x.mp3\n");
#  $output=`mpgedit $silent -o +$outfile -e- descramble_x.mp3`;
#  ($verbose==1) && print("$output");
  print("          done.\n");
  return 0;
}


sub has_xing_header($)
{
  my ($mp3file) = @_;

  # Searching for text pattern this short in binary data
  # may be dangerous.
  #
  open(DDFP, "$mp3file") || die("Failed opening $mp3file");
  binmode(DDFP);
  read(DDFP, $buf, 64) || die("Failed reading header from $mp3file");
  close DDFP;
  ($buf =~ /Xing/) && return "Xing";
  ($buf =~ /Info/) && return "Info";

  return 0;
}




1;

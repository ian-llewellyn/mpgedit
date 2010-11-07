#!/usr/bin/perl
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
use mpgtests;

#$mpgedit="$HOME/wrk/mpgedit/production/mpgedit_0.5_linux/mpgedit";
#$mpgedit="$HOME/wrk/mpgedit/production/mpgedit_0.6/linux/mpgedit";
$mpgedit="./mpgedit";

$errcnt=0;

sub signal_handler()
{
    test4_cleanup();
}

sub main()
{
  #
  # Signal handler
  #
  $SIG{INT} = \&signal_handler;  # best strategy

  $tn=1;
  test4_cleanup();
  test4_1("mpg1_layer1_fl1.mp3",    "384", "49", "28224");
  test4_1("mpg1_layer1_fl7.mp3",    "384", "63", "26332");
  test4_1("mpg1_layer2_fl11.mp3",   "192", "49", "30720");
  test4_1("mpg1_layer2_fl16.mp3",   "256", "63", "48384");
  test4_1("mpg2_layer2_test24.mp3", "96",  "27", "23328");
  test4_2("test1.mp3",              "48",  "224", "144", "1264", "596332");
  test4_1("ATrain23.mp3", "64",     "896", "187245");
  print("\n");
  if ($errcnt == 0) {
    print("Test $tn: SUCCESS: Correctly parsed all input files\n");
  }
  else {
    print("Test $tn: ERROR: Failed to correctly parse $errcnt files\n");
  }
  test4_cleanup();
}


main();

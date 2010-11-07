#!/usr/bin/perl
# 
# See write up in scramble_main() in mpgtests.pm  for a full explanation
# of what this script acutally does.
#

require "mpgtests.pm";

$mpgedit = "./mpgedit";
$silent  = "-s";


my $filename, $option, $verbose;
$verbose=0;

if ($#ARGV == 0) {
  $filename = $ARGV[0];
  $_ = "$filename";
  if (/^-v/) {
    $verbose=1;
    $filename = "scramble.mp3";
  }
}
elsif ($#ARGV == 1) {
  $option    = $ARGV[0];
  $filename = $ARGV[1];
  $_ = "$option";
  if (/^-v/) {
    $verbose=1;
  }
  else {
    print("ERROR: unknown option '$option'\n");
    exit(1);
  }
}
else {
  $filename = "scramble.mp3";
}
unscramble_main($filename, $verbose);
unscramble_file_cleanup();

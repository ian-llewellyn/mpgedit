#!/usr/bin/perl
sub randomize(@)
{
  my (@tarray) = @_;
  my $len, $cnt, $rval1, $rval2, $tmp;

  $len = @tarray;
  $cnt = $len * 5;
  while ($cnt > 0) {
    $rval1 = int(rand($len));
    $rval2 = int(rand($len));
    $tmp = $tarray[$rval1];
    $tarray[$rval1] = $tarray[$rval2];
    $tarray[$rval2] = $tmp;
    $cnt--;
  }
  return @tarray;
}


sub load_times_array($)
{
  my ($file) = @_;
  my @vvoutput, @times, $prev, $i, $str;

  @vvoutput=`mpgedit -vv "$file"`;
  $prev = "0.000";
  $i=1;
  for (@vvoutput) {
    chop;
    if (/^t=/) {
      s/s.*//;
      s/t=//;
      $str = "$i: ${prev}-$_";
      $prev = "$_";
      push(@times, $str);
      $i++;
    }
  }
  return @times;
}


# --------------- main ------------------
if ($#ARGV == -1) {
  print("usage: $0 mp3_file\n");
  exit(1);
}

$ENV{"PATH"} = ".:" . $ENV{"PATH"};

# Seed random number generator with known value; for
# debugging only!!!
#
#srand(1);

@times = load_times_array("$ARGV[0]");
@times = randomize(@times);
if ($#ARGV > 0) {
  open(FP, ">$ARGV[1]") || die ("failed opening output file '$ARGV[1]'\n");
  for (@times) {
    print(FP "$_\n");
  }
  close(FP);
}
else {
  for (@times) {
    print("$_\n");
  }
}

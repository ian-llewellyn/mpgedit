#!/usr/bin/perl
require POSIX;

@tm     = localtime(time);
$YEAR  = POSIX::strftime('%Y', @tm);
$MONTH = POSIX::strftime('%B', @tm);
$DAY   = POSIX::strftime('%d', @tm);

#
# Script to format README into 
# #define XMPGEDIT_CREDITS_COPYRIGHT \
# " blah blah blah\n" \
# and so on.  This is used to create the file
# include credits.h, included by player.c.

open(FP, "../version.h") || die("failed opening version.h");
while(<FP>) {
    if (/ MPGEDIT_VERSION /) {
        chop;
	($MAJOR   = $_) =~ s|.*"(.*)".*|$1|;
        ($MINOR   = $_) =~ s|.*"(.*)".*|$1|;
        ($RELTYPE = $_) =~ s|.*"(.*)".*|$1|;
        ($CKPT    = $_) =~ s|.*"(.*)".*|$1|;
        $MAJOR   =~ s|(\d)\.(\d+) ([a-z]+)(\d*)|$1|;
        $MINOR   =~ s|(\d)\.(\d+) ([a-z]+)(\d*)|$2|;
        $RELTYPE =~ s|(\d)\.(\d+) ([a-z]+)(\d*)|$3|;
        $CKPT    =~ s|(\d)\.(\d+) ([a-z]+)(\d*)|$4|;
    }
    elsif(/ XMPGEDIT_VERSION /) {
        chop;
	($XMAJOR   = $_) =~ s|.*"(.*)".*|$1|;
        ($XMINOR   = $_) =~ s|.*"(.*)".*|$1|;
        ($XRELTYPE = $_) =~ s|.*"(.*)".*|$1|;
        ($XCKPT    = $_) =~ s|.*"(.*)".*|$1|;
        $XMAJOR   =~ s|(\d)\.(\d+) ([a-z]+)(\d*)|$1|;
        $XMINOR   =~ s|(\d)\.(\d+) ([a-z]+)(\d*)|$2|;
        $XRELTYPE =~ s|(\d)\.(\d+) ([a-z]+)(\d*)|$3|;
        $XCKPT    =~ s|(\d)\.(\d+) ([a-z]+)(\d*)|$4|;
    }
}
close(FP);

$RELEASE = "${MAJOR}-${MINOR}${RELTYPE}${CKPT}";
$XRELEASE = "${XMAJOR}-${XMINOR}${XRELTYPE}${XCKPT}";
#print("RELEASE=$RELEASE\n");
#print("XRELEASE=X$RELEASE\n");
#print("MAJOR=$MAJOR\n");
#print("MINOR=$MINOR\n");
#print("RELTYPE=$RELTYPE\n");
#print("CKPT=$CKPT\n");
#print("XMAJOR=$XMAJOR\n");
#print("XMINOR=$XMINOR\n");
#print("XRELTYPE=$XRELTYPE\n");
#print("XCKPT=$XCKPT\n");
#exit(0);
open(FP, "../README") || die("failed opening README");

$first = 1;
while(<FP>) {
  if ($first) {
    print("#define XMPGEDIT_CREDITS_COPYRIGHT \\\n");
    print('"                        Credits\\n\\n" \\' . "\n");
    $first = 0;
  }
  s|"|\\"|g;
  s|.*|"$&\\n" \\|;
  s|__RELEASE__|$RELEASE|g;
  s|__MAJOR__|$MAJOR|g;
  s|__MINOR__|$MINOR|g;
  s|__RELTYPE__|$RELTYPE|g;
  s|__CKPOINT__|$CKPT|g;
  s|__XRELEASE__|$XRELEASE|g;
  s|__XMAJOR__|$XMAJOR|g;
  s|__XMINOR__|$XMINOR|g;
  s|__XRELTYPE__|$XRELTYPE|g;
  s|__XCKPOINT__|$XCKPT|g;
  s|__MONTH__|$MONTH|g;
  s|__DAY__|$DAY|g;
  s|__YEAR__|$YEAR|g;

  print $_;
}
print("\"\"\n");
close(FP);

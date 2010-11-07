#!/usr/bin/perl

require "mpgtests.pm";
use File::Compare;

$mpgedit = "./mpgedit";
$silent  = "-s";

sub test2_cleanup()
{
    my $i, $rmfile;
    
    # Cleanup .mp3 edit files
    $i=1;
    $rmfile = "${base}_${i}.${ext}";
    while (-f "$rmfile") {
        unlink("$rmfile");
        $i++;
        $rmfile = "${base}_${i}.${ext}";
    }

    # Cleanup .idx edit files
    $i = 1;
    $rmfile = "${base}_${i}.idx";
    while (-f "$rmfile") {
        unlink("$rmfile");
        $i++;
        $rmfile = "${base}_${i}.idx";
    }

    # Cleanup the mpgedit -vvv output file
    $rmfile = "${base}.times";
    unlink("$rmfile");

    # Cleanup the splice file
    $rmfile = "${base}_splice.${ext}";
    unlink("$rmfile");

    # Cleanup the input .idx file
    $rmfile = "${base}.idx";
    unlink("$rmfile");
}


sub signal_handler {
    my $signame = shift;
    $shucks++;
    test2_cleanup();
    exit(1);
}

#
# Signal handler
#
$SIG{INT} = \&signal_handler;


sub main()
{
    $edit_file = $#ARGV >=0 ? $ARGV[0] : "test1.mp3";
    ($base = $edit_file)  =~ s/\..*//;
    ($ext  = $edit_file)  =~ s/.*\.//;
    $tn = $ENV{'MPGEDIT_TEST_NUMBER'};
    $tn = (length("$tn") == 0)  ? 1 : $tn;

    test2_cleanup();
    slice_and_splice("$base", "$ext");
    test2_cleanup();
}


main();

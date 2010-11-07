#!/usr/bin/perl
#
# Map date string of format:   February 17 2006
# into date string of format:  Fri Feb 17 2006
# The second form is what RPM requires in the change log section.
#

require POSIX;

sub map_string_date($)
{
    my ($in_date) = @_;
    my $str_month;
    my $month;
    my $day;
    my $year;
    my $map_date;
    my %months;

    ($str_month = $in_date) =~ s/(\w+) +.*/$1/; 
    ($year      = $in_date) =~ s/(\w+).* +(\d+)/$2/;
    ($day       = $in_date) =~ s/(\w+) +(\d\d?) +(\d+)/$2/;
    if ($day == $in_date) {
        $day = "1";
    } 

#    print("Month='$str_month'\n");
#    print("Day=$day\n");
#    print("Year=$year\n");
#    print("=================\n");

    %months = ("January",   1, "February", 2, "March",      3, "April",     4,
               "May",       5, "June",     6, "July",       7, "August",    8,
               "September", 9, "October",  10, "November", 11, "December", 12);

    $month = $months{$str_month};
    if (length($month) == 0) {
        return("ERROR");
    }
    
    $month = $month - 1;
    $year  = $year - 1900;

    $map_date = POSIX::strftime("%a %b %d %Y", 0, 0, 0, $day,
                                $month, $year, -1, -1, -1);
    return ($map_date);
}

#$d = "February 17 2006";
#$val = map_string_date($d);
#print("Date: $d -> $val\n");

if ($#ARGV == -1) {
    print("usage: $0 filename\n");
    exit(1);
}
$infile=$ARGV[0];

open(FP, $infile) || die("opening file '$infile'");
$date_found = 0;
while (<FP>) {
    if (/\*version/) {
        $date_found = 1;
        chop;
        ($indate = $_) =~ s/\*version +[^\s]+ +//;
        $indate        =~ s/- ?//;
        $indate        =~ s/.*\) ?//;
        $indate        =~ s/ *\*//;
        $indate        =~ s/ *\*$//;
        $map_date = map_string_date($indate);
        print("* $map_date *\n");
    }
    elsif (/\t                    \t/) {
        exit(0);
    }
    elsif ($date_found == 1) {
        $date_found = 0;
        s/( +)-/  */;
        print;
    }
    else {
        s/( +)-/$1*/;
        print;
    }
}

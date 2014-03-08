#!/usr/bin/perl
# USAGE: ./genlines.pl allpkts.csv

use strict;
use warnings;

# Converts spreadsheet containing packet counts into an easier form to plot
# format of file:
#   Node | Packets received from 0 | Packets received from 1 | ... | Packets received from (n-1)
# Outputs in format:
#   StartX,StartY,EndX,EndY,Strength

######## CONSTANTS ########
my $NODES_X = 9;   # number of nodes in x direction
my $NODES_Y = 9;   # number of nodes in y direction
my $SPACING = 30;  # metres

########## BEGIN MAIN BODY OF SCRIPT ##########
(my $infile, my $outfile) = ($ARGV[0], $ARGV[1]);
my $time = time();
my $intermediateFile = "/tmp/genlines-${time}.csv";

open(my $INFH, "<", $infile) or die "Failure opening < ${infile}: $!";
open(my $TMPFH, ">", $intermediateFile) or die "Failure opening < ${intermediateFile}: $!";
open (my $OUTFH, ">", $outfile) or die "Failure opening < ${outfile}: $!";


# load (x,y) coordinates
my @xcoords;
my @ycoords;
my $n = 0;
my $x, my $y;
for ($y=0; $y<$NODES_Y*$SPACING; $y+=$SPACING) {
    for ($x=0; $x<$NODES_X*$SPACING; $x+=$SPACING) {
	($xcoords[$n], $ycoords[$n]) = ($x, $y);
	$n++;
    }
}


$n=0; my $tmp;
my $m=0;
my $startx, my $starty, my $endx, my $endy;
my @stations; my $station;
<$INFH>; # skip first line
while (<$INFH>) {
    chomp;
    ($endx, $endy) = ($xcoords[$n], $ycoords[$n]);
    @stations = split(/,/);
    foreach (@stations) {
	if ($m != 0) {   # skip title column
	    #($startx, $starty) = ($xcoords[$m], $ycoords[$m]);
	    $startx = $xcoords[$m-1];
	    $starty = $ycoords[$m-1];
	    $station = $stations[$m];
	    print $TMPFH "${startx} ${starty} $station\n";
	    print $TMPFH "${endx} ${endy} $station\n\n";
	}
	$m++;
    }
    $m=0;
    $n++;
}
close $INFH; # finished with it

# now go through and clean lines that are just 0
close $TMPFH;
open($TMPFH, "<", $intermediateFile) or die "Failure opening < ${intermediateFile}: $!";
my @line; my $l; my $s;
while (<$TMPFH>) {
    $l = $_;
    @line = split(/ /);
    $s = @line;
#    if ($s > 1 && $line[2] != 0) {
	print $OUTFH $l;
#    } elsif ($s <= 1) {
#	print $OUTFH "\n";
#    }
}


close $TMPFH;
close $OUTFH;

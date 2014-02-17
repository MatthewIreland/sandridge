#!/usr/bin/perl
# USAGE: ./numhops.pl <castaliatrace>

use strict;
use warnings;
use POSIX;

my $tracefile = $ARGV[0];

(my $sec, my $min, my $hour, my $mday, my $mon, my $year, my $wday, my $yday, my $isdst) = localtime(time);
my $tmpfilename = "/tmp/numhops-${mday}-${mon}$-${hour}$-${min}-${sec}";

open (my $TRACEFH, "<", $tracefile) or die "Failure opening < ${tracefile}: $!";
open (my $TMPFH, ">", $tmpfilename) or die "Failure opening temporary file for writing.";

# initialise min hop array
my @minhops = (4, 4, 4, 4, 4, 4, 4, 4, 4,
	       4, 3, 3, 3, 3, 3, 3, 3, 4,
	       4, 3, 2, 2, 2, 2, 2, 3, 4,
	       4, 3, 2, 1, 1, 1, 2, 3, 4,
	       4, 3, 2, 1, 0, 1, 2, 3, 4,
	       4, 3, 2, 1, 1, 1, 2, 3, 4,
	       4, 3, 2, 2, 2, 2, 2, 3, 4,
	       4, 3, 3, 3, 3, 3, 3, 3, 4,
	       4, 4, 4, 4, 4, 4, 4, 4, 4);

# initialise packet proportions within nk hops
my %packetproportions;

# pass 1 - strip unneeded data
my $line;
while (<$TRACEFH>) {
    $line = $_;
    if (/^.*SINK.*$/) {
	print $TMPFH $line;
    }
}

close $TRACEFH;
close $TMPFH;

open ($TMPFH, "<", $tmpfilename) or die "Failure opening temporary file for reading.";
my $nk = -1;
my $nodenumber = -1;
my $hopcount = -1;
my $packetseqnum = -1;
my $minhops = -1;
while (<$TMPFH>) {
    $line = $_;
    # 299.569008963911SN.node[40].Communication.Routing        SINK received packet 5 from 64 in 5 hops.
    if (/^.*packet\s(\d+)\sfrom\s(\d+)\sin\s(\d+)\shops.*$/) {
	$packetseqnum = $1;
	$nodenumber = $2;
	$hopcount = $3;
	$minhops = $minhops[$nodenumber];

	# now round UP (since we're interested in *within* n hops)
	$nk = ceil($hopcount / $minhops);

#	print "Packet: node $nodenumber hopcount $hopcount minhops $minhops nk $nk \n";

	$packetproportions{$nk}++;
    } else {
	print "ERROR! DID NOT MATCH.\n";
    }
}

close $TRACEFH;

#print "%%% TOTAL PACKET COUNTS %%%\n";
#print "$_ => $packetproportions{$_}\n" for sort {$a <=> $b} keys %packetproportions;
#print "\n\n";

print "%%% PROPORTION OF PACKETS RECEIVED WITHIN nk HOPS %%%\n";
my $totalpackets = 0;
my $n;
foreach $n ( keys %packetproportions ) {
    $totalpackets += $packetproportions{$n};
}
my $proportion;
foreach $n ( sort {$a <=> $b} keys %packetproportions ) {
    $proportion = $packetproportions{$n}/$totalpackets;
    print "$n => $proportion\n";
}

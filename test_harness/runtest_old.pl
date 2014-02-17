#!/usr/bin/perl
# USAGE: ./runtest.pl <numberofnodes> <validtransitionsfile> <castaliaoutput>

use strict;
use warnings;

########## BEGIN MAIN BODY OF SCRIPT ##########
# parse input arguments
(my $numNodes, my $transfile, my $logfile) = ($ARGV[0], $ARGV[1], $ARGV[2]);

# open files for reading
open(my $TRANSFH, "<", $transfile) or die "Failure opening < ${transfile}: $!";
open(my $LOGFH, "<", $logfile) or die "Failure opening < ${logfile}: $!";
# TODO: read in valid state transitions

my @transitions = <$TRANSFH>;

my $nodeNumber;
my $log_line;
my $transitionsValid = 1;
my $doesNotMatchTransition = 1;
my $currentTest;
my $lhs;
my $rhs;
# e.g. 0.253537943757  SN.node[1].Communication.MAC             XMAC TRANSITION: XMAC_STATE_SLEEP to XMAC_STATE_CCA.
#for ($nodeNumber=0; $nodeNumber<$numNodes; $nodeNumber++) {
#    open(my $LOGFH, "<", $logfile) or die "Failure opening < ${logfile}: $!";
    while(<$LOGFH>) {
	chomp;
	$log_line = <$LOGFH>;
	if ($log_line =~ /.*TRANSITION:.*/) {
	    $doesNotMatchTransition = 1;
	    $log_line =~ /.*TRANSITION: (.*) to (.*)\./;
#	    $log_line =~ /.*TRANSITION: (.*)/;
#	    print "Transition from $1 to $2 \n";
	    $lhs = $1;
	    $rhs = $2;
	    foreach (@transitions) {
		$currentTest = $_;
		$currentTest =~ /(.*),(.*)/;
		if (($1 eq $lhs) && ($2 eq $rhs)) {
		    $doesNotMatchTransition = 0;
		}
	    }
	    if ($doesNotMatchTransition == 0) {
		print "TRANSITION ERROR: ${log_line}\n";
		$transitionsValid = 0;
	    }
	}
    }
    close ($LOGFH);
#}

if ($transitionsValid) {
    print "\n\nAll transitions valid";
}

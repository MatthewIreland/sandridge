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

my @transitions = <$TRANSFH>;

my $nodeNumber;
my $log_line;
my $transitionsValid = 1;
my $doesNotMatchTransition = 1;
my $currentTest;
my $lhs;
my $rhs;

while(<$LOGFH>) {
    chomp;
    $log_line = $_;
    if ($log_line =~ /.*TRANSITION:.*/) {
	$doesNotMatchTransition = 1;
	$log_line =~ /.*TRANSITION: (.*) to (.*)\./;
	$lhs = $1;
	$rhs = $2;
	foreach (@transitions) {
	    $currentTest = $_;
	    $currentTest =~ /(.*),(.*)/;
	    if (($1 eq $lhs) && ($2 eq $rhs)) {
		$doesNotMatchTransition = 0;
	    }
	}
	if ($doesNotMatchTransition) {
	    print "TRANSITION ERROR: ${log_line}\n";
	    $transitionsValid = 0;
	}
    }
}
close ($LOGFH);

if ($transitionsValid) {
    print "\n\nAll transitions valid\n";
}

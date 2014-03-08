#!/usr/bin/perl
# USAGE: ./gengrid.pl > omnetpp.ini

use strict;
use warnings;

######## CONSTANTS ########
my $SPACING = 30;  # metres
my $NODES_X = 9;   # number of nodes in x direction (must be odd)
my $NODES_Y = 9;   # number of nodes in y direction (must be odd)
my $TIME_LIMIT = 300;  # seconds


######## MAIN BODY OF SCRIPT ########

print "[General]\n\n";
print "include ../Parameters/Castalia.ini\n\n";
print "sim-time-limit = ${TIME_LIMIT}s\n\n";
my $fieldx = $NODES_X * $SPACING;
my $fieldy = $NODES_Y * $SPACING;
print "SN.field_x = $fieldx      \# metres\n";
print "SN.field_y = $fieldy      \# metres\n";
my $n = $NODES_X * $NODES_Y;
print "SN.numNodes = $n\n\n";

my $x; my $y;
$n = 0;
my $row = 1;

for ($y=0; $y<$NODES_Y*$SPACING; $y+=$SPACING) {
    print "\# row $row\n";
    for ($x=0; $x<$NODES_X*$SPACING; $x+=$SPACING) {
	print "SN.node[${n}].xCoor = $x\n";
	print "SN.node[${n}].yCoor = $y\n";
	$n++;
    }
    print "\n";
    $row++;
}

print "SN.node[*].Communication.Radio.RadioParametersFile = \"../Parameters/Radio/CC2420.txt\"\n";
print "SN.node[*].Communication.RoutingProtocolName = \"RandomRouting\"\n";
print "SN.node[*].Communication.MACProtocolName = \"TunableMAC\"\n\n";

print "SN.node[*].ApplicationName = \"ValueReporting\"\n";
print "SN.node[*].Communication.Radio.txPowerLevelUsed = 2\n";
print "SN.node[*].Communication.Routing.collectTraceInfo = true\n\n";

print "SN.node[*].Communication.MAC.collectTraceInfo = false\n\n";

my $sink = ($n-1)/2;  # assuming symmetric grid
print "SN.node[${sink}].Application.isSink = true\n";

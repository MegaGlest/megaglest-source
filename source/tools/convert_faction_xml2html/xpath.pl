#!/usr/bin/perl -w

# Copyright 2011 olaus
# license: GPLv3 or newer
# show xpath in file, google xpath for syntax


if ( !@ARGV ) {
	die "\n\nusage $0 file xpath\n\n";
}

use strict;
use XML::XPath;
use XML::XPath::XMLParser;

my $xpath = XML::XPath->new( filename => shift @ARGV );

my $nodeset = $xpath->find( shift @ARGV );

foreach my $node ( $nodeset->get_nodelist ) {
        print XML::XPath::XMLParser::as_string( $node ) . "\n";
}


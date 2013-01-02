#!/usr/bin/perl -w

# utility for megaglest, probably works for glest and GAE too
# merge new language variables from the main language (english) to all other language files
# Copyright 2011 olaus, 20101222
# license: GPL v3

##########################################

use strict;

my $now = &gettime();

if ( $#ARGV != 1 ) {
	die "\nusage: $0 directory-to-create main-language-file\n\n";
}


our %main;
our @main_line;

our $self = $0;
$self =~ s;^.*/([^/]*)$;$1;g;
# print "self: $self\n";
# die;

our $dir = $ARGV[0];
our $main_lang = $ARGV[1];
mkdir $dir;

opendir(DIR, "." ) || die "cant open directory\n";
my @files = grep { /\.lng$/ } readdir(DIR);
closedir(DIR);

# read main-master-development-language
open (MAIN, "< $main_lang") || die "can't read main-language-file: $main_lang ERROR: $!\n";
my $row=0;
while(my $line =<MAIN>) {
	
	chomp $line;

	# comment? or empty?
	if ( $line =~ /^;/ || $line =~ /^\s*$/ ) {
		# ignore
	}
	# f.e.: UnitAddedToProductionQueue=Unit added to production queue
	elsif ( $line =~ /(.+?)=(.+)/ ) {
		my $var   = $1;
		my $value = $2;
		$main{ $var } = $value;
		$main_line[ $row++ ] = "$line\n";
	}
	else {
		print "UNKNOWN LINE: $line\n";
	}
}

close (MAIN);

LNGS: foreach my $file ( @files ) {

	# skip main-language-file
	next LNGS if ( $file eq $main_lang);

	open (LNG, "< $file") || die "cant read $file\n";
	open (OUT, "> $dir/$file" ) || die "cant write $dir/$file - $!\n";

	my %lng;
	my %other_lines;
	my $other_line;

	my $head_line =<LNG>;
	print OUT $head_line;

	# read each lng-file
	while(my $line = <LNG>) {

		chomp $line;

		# comment?
		# f.e.: UnitAddedToProductionQueue=Unit added to production queue
		if ( $line =~ /(.+?)=(.+)/ ) {
			my $var   = $1;
			my $value = $2;
			$lng{ $var } = $value;

			# save comments:
			$other_lines{ $var } = $other_line;
			$other_line ="";
		}
		elsif ( $line =~ /^;/ || $line =~ /^\s*$/ ) {
			$other_line .= "$line\n";
			# ignore
		}
		
	}

	# loop over main-lng-file in including missing vars in lng-file
	for $row (0..$#main_line) {

		$main_line[ $row ] =~ /(.+?)=(.+)/;
		my $var   = $1;
		my $value = $2;


		# keep comments and empty lines from main-lng-file
		if ( $main_line[ $row ] =~ /^;/ || $main_line[ $row ] =~ /^\s*$/ ) {
			print OUT $main_line[ $row ];
		}
		# var already in lng-file, keep it
		elsif ( $lng{ $var } ) {
			# and keep comments if any
			if ( $other_lines{ $var } ) {
				print OUT $other_lines{ $var };
			}
			print OUT "$var=".$lng{ $var }."\n";
		}
		# new var not in lng-file, include it from main-lng
		else {
			print OUT "; Next line needs translation, include by $self on $now\n";
			print OUT $main_line[ $row ];
		}
	}

	close (OUT);
	close (LNG);
}

sub gettime {
	my @months = qw(Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec);
	my @weekDays = qw(Sun Mon Tue Wed Thu Fri Sat Sun);
	my ($second, $minute, $hour, $dayOfMonth, $month, $yearOffset, $dayOfWeek, $dayOfYear, $daylightSavings) = localtime();
	my $year = 1900 + $yearOffset;
	my $theTime = "$weekDays[$dayOfWeek] $months[$month] $dayOfMonth, $year";
	return $theTime; 
}


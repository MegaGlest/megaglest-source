#!/usr/bin/perl

use strict;
use Shell::Parser;
    
my $sawnewline = 1;
my $sawwhitespace = 1;
my $funccount = 0;
my $braces = 0;
my $currentfn = undef;
my %funcmap;
my %funcstart;
my %funcend;
my %funcused;
my @output;

my $parser = new Shell::Parser handlers => { default => \&dumpnode };
$parser->parse(join '', <>);
    
sub dumpnode {
    my $self = shift;
    my %args = @_;

    my $token = $args{token};
    my $type = $args{type};

    if (($type eq 'comment') or ($token eq '')) {
        return;
    } elsif ($token eq "\n") {
        return if ($sawnewline);
        $sawnewline = 1;
        $sawwhitespace = 1;
        push @output, "\n";
        return;
    } elsif ($token =~ /\A\s+\Z/) {
        return if ($sawwhitespace);
        $sawwhitespace = 1;
        push @output, ' ';
        return;
    } else {
        $sawnewline = 0;
        $sawwhitespace = 0;

        # shrink function names down to "fX"
        if ($token eq ')') {
            my $prev1 = pop @output;
            if ((not defined $prev1) or ($prev1 ne '(')) {
                push @output, $prev1;
            } else {
                my $prev2 = pop @output;
                if (defined $prev2) {
                    $funccount = $funccount + 1;
                    my $mappedfn = "f${funccount}";
                    $currentfn = $prev2;
                    $funcmap{$currentfn} = $mappedfn;
                    $funcused{$currentfn} = 0;
                    $funcstart{$currentfn} = scalar(@output);
                    $prev2 = $mappedfn;
                }
                push @output, $prev2;
                push @output, $prev1;
            }
        } elsif ($token eq '{') {
            $braces++;
        } elsif ($token eq '}') {
            $braces--;
            if (($braces == 0) and (defined $currentfn)) {
                $funcend{$currentfn} = scalar(@output) + 1;
                $currentfn = undef;
            }
        } elsif (defined $funcmap{$token}) {
            $funcused{$token} = 1;
            $token = $funcmap{$token};
        }

        push @output, $token;
    }
}

foreach my $fn (keys(%funcused)) {
    #print STDERR "funcused{'" . $fn . "'} == '" . $funcused{$fn} . "';\n";
    #print STDERR "funcstart{'" . $fn . "'} == '" . $funcstart{$fn} . "';\n";
    #print STDERR "funcend{'" . $fn . "'} == '" . $funcend{$fn} . "';\n";
    #print STDERR "\n";
    next if $funcused{$fn};
    my $fnstart = $funcstart{$fn};
    my $fnend = $funcend{$fn};
    my $len = $fnend - $fnstart;
    for (my $i = $fnstart; $i <= $fnend; $i++) {
        $output[$i] = undef;
    }
    #print STDERR "Removed unused function $fn ($len tokens)\n";
}

foreach (@output) {
    print($_) if defined $_;
}


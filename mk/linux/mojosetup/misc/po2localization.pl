#!/usr/bin/perl -w

use warnings;
use strict;
use Encode qw( decode_utf8 );

# Fixes unicode dumping to stdio...hopefully you have a utf-8 terminal by now.
binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");
binmode(STDERR, ":utf8");

my $now = `date '+%Y-%m-%d %H:%M:%S%z'`;
chomp($now);

my $hgver = `hg tip --template 'hg-{rev}:{node|short}' 2>/dev/null`;
$hgver = '???' if ($hgver eq '');

my %languages;
my %comments;
my %msgstrs;
my @strings;
my $saw_template = 0;
my $exportdate = '';
my $generator = '';

foreach (@ARGV) {
    my $fname = $_;
    my $template = /\.pot\Z/;

    open(POIO, '<', $fname) or die("Failed to open $_: $!\n");
    binmode(POIO, ":utf8");

    if ($template) {
        die("multiple .pot files specified\n") if ($saw_template);
        $saw_template = 1;
    }

    my $comment = '';
    my $currentlang = '';

    while (<POIO>) {
        chomp;
        s/\A\s+//;
        s/\s+\Z//;
        next if ($_ eq '');

        if (s/\A\#\.\s*(.*)\Z/$1/) {
            if ($template) {
                my $txt = $_;
                $txt = " $txt" if ($comment ne '');
                $comment .= "    -- $txt\n";
            }
            next;
        }

        next if /\A\#/;

        if (s/msgid\s*\"(.*?)\"\Z/$1/) {
            if (($_ eq '') and ($currentlang eq '')) {   # initial string.
                while (<POIO>) {  # Skip most of the metadata.
                    chomp;
                    s/\A\s+//;
                    s/\s+\Z//;
                    last if ($_ eq '');
                    if (/\A\"Language-Team: (.*?) \<(.*?)\@.*?\>\\n"\Z/) {
                        $currentlang = $2;
                        if (defined $languages{$currentlang}) {
                            die("Same language twice: $currentlang\n");
                        } elsif ($currentlang eq 'en') {
                            die("Found an 'en' translation.\n");
                        } elsif ($currentlang eq 'en_US') {
                            die("Found an 'en_US' translation.\n");
                        }
                        $languages{$currentlang} = $1 if (not $template);
                    } elsif (s/\A\"(X-Launchpad-Export-Date: .*?)\\n\"/$1/) {
                        $exportdate = $_ if ($template);
                    } elsif (s/\A"(X-Generator: .*?)\\n\"\Z/$1/) {
                        $generator = $_ if ($template);
                    }
                }
            } elsif ($currentlang eq '') {
                die("No current language!\n");
            } else {  # new string
                my $msgstr = '';
                my $msgid = $_;
                while (<POIO>) {   # check for multiline msgid strings.
                    chomp;
                    s/\A\s+//;
                    s/\s+\Z//;
                    if (s/\Amsgstr \"(.*?)\"\Z/$1/) {
                        $msgstr = $_;
                        last;
                    }
                    if (s/\A\"(.*?)\"\Z/$1/) {
                        $msgid .= $_;
                    } else {
                        die("unexpected line: $_\n");
                    }
                }
                while (<POIO>) {   # check for multiline msgstr strings.
                    chomp;
                    s/\A\s+//;
                    s/\s+\Z//;
                    last if ($_ eq '');
                    if (s/\A\"(.*?)\"\Z/$1/) {
                        $msgstr .= $_;
                    } else {
                        die("unexpected line: $_\n");
                    }
                }

                if ($template) {
                    push @strings, $msgid;  # This is a list, to keep original order.
                    $comments{$msgid} = $comment;
                    $comment = '';
                } elsif ($msgstr ne '') {
                    $msgstrs{$currentlang}{$msgid} = $msgstr;
                }
            }
        }
    }

    close(POIO);
}

die("no template seen\n") if (not $saw_template);


print <<__EOF__;
-- MojoSetup; a portable, flexible installation application.
--
-- Please see the file LICENSE.txt in the source's root directory.
--
-- DO NOT EDIT BY HAND.
-- This file was generated with po2localization.pl, version $hgver ...
--  on $now
--
-- Your own installer's localizations go into app_localization.lua instead.
-- If you want to add strings to be translated to this file, contact Ryan
-- (icculus\@icculus.org). If you want to add or change a translation for
-- existing strings, please use our nice web interface here for your work:
--
--    https://translations.launchpad.net/mojosetup/
--
-- ...and that work eventually ends up in this file.
--
-- $exportdate
-- $generator

MojoSetup.languages = {
__EOF__

print "    en_US = \"English (United States)\"";

foreach (sort keys %languages) {
    my $k = $_;
    my $v = $languages{$k};
    print ",\n    $k = \"$v\""
}
print "\n};\n\nMojoSetup.localization = {";

foreach (@strings) {
    my $msgid = $_;
    print "\n";
    print $comments{$msgid};
    print "    [\"$msgid\"] = {\n";
    my $first = 1;
    foreach (sort keys %languages) {
        my $k = $_;
        my $str = $msgstrs{$k}{$msgid};
        next if ((not defined $str) or ($str eq ''));
        print ",\n" if (not $first);
        print "        $k = \"$str\"";
        $first = 0;
    }
    print "\n    };\n";
}

print "};\n\n-- end of localization.lua ...\n\n";

# end of po2localization.pl ...


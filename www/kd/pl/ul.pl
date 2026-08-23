#!/usr/bin/perl

use strict;
use warnings;
use CGI;


my $cgi = CGI->new;

my $g1 = $cgi->param('g1');
my $g2 = $cgi->param('g2');

my $p1 = $cgi->param('p1');
my $p2 = $cgi->param('p2');

my $f = $cgi->param('file');

print("Content-Type: text/plain\r\n\r\n");

print("Perl : UPLOAD!\n");

if ($f)
{
    print("\nFILE\n");
    print($f, "\n");

    my $fp = $cgi->upload('file');
    print($fp);
    open UPLOADFILE, ">./upload-pl-$f";
    binmode UPLOADFILE;
    while ( <$fp> ) { print UPLOADFILE; }
    close UPLOADFILE;
}
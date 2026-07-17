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
print("Perl : hello, world!\n");
print("\nGET VARS\n"); # not if POST
print ("g1 : ", $g1, "\n");
print ("g2 : ", $g2, "\n");

print("\nPOST VARS\n");
print ("p1 : ", $p1, "\n");
print ("p2 : ", $p2, "\n");

print ("\nENV\n\n");
while (my ($k,$v)=each %ENV)
{
    print "$k = $v\n";
}
print("\n");

if ($f)
{
    print("\nFILE\n");
    print($f, "\n");

    my $fp = $cgi->upload('file');
    print($fp);
    open UPLOADFILE, ">./$f";
    binmode UPLOADFILE;
    while ( <$fp> ) { print UPLOADFILE; }
    close UPLOADFILE;
}
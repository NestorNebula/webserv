#!/usr/bin/perl

use strict;
use warnings;
use CGI;


my $cgi = CGI->new;

my $g1 = $cgi->param('g1');
my $g2 = $cgi->param('g2');

my $p1 = $cgi->param('p1');
my $p2 = $cgi->param('p2');


print("Perl : hello, world!\n");
print("\nGET VARS\n");
print ($g1);
print ("\n");
print ($g2);
print ("\n");

print("\nPOST VARS\n");
print ($p1);
print ("\n");
print ($p2);
print ("\n");

print ("\nENV\n");
while (my ($k,$v)=each %ENV)
{
    print "$k = $v\n";
}
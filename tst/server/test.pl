#!/usr/bin/perl

use strict;
use warnings;
use CGI;


my $qs = $ENV{'QUERY_STRING'};

my $cgi = CGI->new;
# CGI->new;
# my %param = map { $_ => scalar $cgi->param($_) } 

my $p1 = $cgi->param('p1');
my $p2 = $cgi->param('p2');

print ($qs);
print ("\n");

print("Perl : hello, world!\n");


print ($p1);
print ("\n");
print ($p2);
print ("\n");




# print qq{PARAM:\N};
# for my $k ( sort keys %param ) {
#     print join ": ", $k, $param{$k};
#     print "\n";
# }

# KEEP_ALIVE
# print("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: Keep-Alive\r\nContent-Length: 21\r\n\r\nPerl : hello, world!\n");

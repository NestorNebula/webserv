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

print ( $qs );


print("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 26\r\n\r\nPerl : hello, world!\n");


print ($p1);
print ($p2);




# print qq{PARAM:\N};
# for my $k ( sort keys %param ) {
#     print join ": ", $k, $param{$k};
#     print "\n";
# }

# KEEP_ALIVE
# print("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: Keep-Alive\r\nContent-Length: 21\r\n\r\nPerl : hello, world!\n");

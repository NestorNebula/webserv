#!/usr/bin/perl

use strict;
use warnings;
use CGI;

my $cgi = CGI->new;

print("Content-Type: text/plain\r\n");
print("Connection: keep-alive\r\n\r\n");
# print("Content-Length: 12\r\n\r\n");

print("PERL is not dead (yet).");

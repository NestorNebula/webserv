#!/usr/bin/perl

use strict;
use warnings;
use CGI;

my $cgi = CGI->new;

print("Content-Type: text/plain\r\n");
# print("Connection: keep-alive\r\n");
# print("Content-Length: 11\r\n");
print("\r\n");
print("PERL is not dead (yet).");

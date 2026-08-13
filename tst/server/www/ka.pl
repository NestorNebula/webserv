#!/usr/bin/perl

use strict;
use warnings;
use CGI;

my $cgi = CGI->new;

print("Content-Type: text/plain\r\n");
print("Connection: keep-alive\r\n");
print("Content-Length: 17\r\n\r\n");

print("I'm not dead yet.");

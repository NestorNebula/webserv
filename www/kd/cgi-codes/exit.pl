#!/usr/bin/perl

use strict;
use warnings;
use CGI;

my $cgi = CGI->new;

my $code = $cgi->param('code');

print("Content-Type: text/plain\r\n\r\n");
print("PERL : will exit (", $code, ")\r\n");
exit ($code);


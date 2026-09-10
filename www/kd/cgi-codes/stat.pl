
use strict;
use warnings;
use CGI;

my $cgi = CGI->new;

my $code = $cgi->param('code');

print("Content-Type: text/plain\r\n");
print("Status: ", $code, "\r\n\r\n");

print("PERL Status: ", $code, "\r\n");


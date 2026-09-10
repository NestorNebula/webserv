use strict;
use warnings;
use CGI;

my $cgi = CGI->new;

print("Content-Type: text/plain\r\n\r\n");

print("before error\r\n");
rint("PERL : code has errors\r\n");
print("You won't see me\r\n");

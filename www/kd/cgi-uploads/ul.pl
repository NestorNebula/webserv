#!/usr/bin/perl

use strict;
use warnings;
use CGI;

my $cgi = CGI->new;

my $f = $cgi->param('file');

print("Content-Type: text/html\r\n\r\n");

print("PERL : upload<br>");
if ($f)
{
    my $path = "./upload-pl-$f";
    if (-e $path)
    {
        print("PERL : file exists<br>");
        print("<a href='javascript:history.back();'>BACK</a><br>");
        exit(0);
    }

    print("file : ", $f, "<br>");
    print("path : ", $path, "<br>");

    my $fp = $cgi->upload('file');
    open UPLOADFILE, ">$path";
    binmode UPLOADFILE;
    while ( <$fp> ) { print UPLOADFILE; }
    close UPLOADFILE;
}
else
{
    print("PERL : no file set<br>");
}
print("<a href='javascript:history.back();'>BACK</a><br>");
#!/usr/bin/perl

use strict;
use warnings;
use CGI;

my $cgi = CGI->new;

my $f = $cgi->param('file');

print("Content-Type: text/plain\r\n\r\n");

print("PERL : upload\n\n");

if ($f)
{
    my $path = "./upload-pl-$f";
    print("file : ", $f, "\n");
    print("path : ", $path, "\n");

    my $fp = $cgi->upload('file');
    open UPLOADFILE, ">$path";
    binmode UPLOADFILE;
    while ( <$fp> ) { print UPLOADFILE; }
    close UPLOADFILE;
}
    else
    {
        print("PERL : no file set\n");
    }
from __future__ import print_function

import sys
import os

import cgi
import cgitb

form = cgi.FieldStorage()

code = form.getvalue("code", "0")

print("Content-Type: text/plain", end="\r\n\r\n");
print("how many lines\r\n");
print("do we get\r\n");

rint("PYTHON : code has errors\r\n");
print("until\r\n");
print("we have a problen\r\n");




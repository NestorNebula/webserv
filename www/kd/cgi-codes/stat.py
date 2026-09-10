from __future__ import print_function

import sys
import os

import cgi
import cgitb

form = cgi.FieldStorage()

code = form.getvalue("code", "200")

print("Content-Type: text/plain", end="\r\n");
print("Status: ", code, end="\r\n\r\n");
print("PYTHON Status: ", code);



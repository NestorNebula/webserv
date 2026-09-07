from __future__ import print_function

import sys
import os

import cgi
import cgitb

form = cgi.FieldStorage()

code = form.getvalue("code", "200")

print("Status: ", code, end="\r\n\r\n");
print("Status: ", code);



from __future__ import print_function

import sys
import os

import cgi
import cgitb

form = cgi.FieldStorage()

code = form.getvalue("code", "0")

print("Content-Type: text/plain", end="\r\n\r\n");
print("PYTHON : will exit (" + code + ")\r\n");

exit (int(code));


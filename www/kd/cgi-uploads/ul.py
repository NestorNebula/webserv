#!/usr/bin/python
from __future__ import print_function

import sys
import os

import cgi
import cgitb

print("Content-Type: text/html", end="\r\n\r\n");

print("PYTHON : upload", end="<br>")

form = cgi.FieldStorage()

if "file" in form:
    file_item = form["file"]
    if file_item.filename:
        file_name = file_item.filename
        file_path = './upload-py-' + file_name

        if os.path.exists(file_path):
            print("PYTHON : file exists<br>");
            print("<a href='javascript:history.back();'>BACK</a><br>");
            exit (0)

        print("file : ", file_name, "<br>")
        print("path : ", file_path, "<br>")
        with open(file_path, 'wb') as file:
            file.write(file_item.file.read())
else:
    print("PYTHON : no file set<br>")

print("<a href='javascript:history.back();'>BACK</a><br>");

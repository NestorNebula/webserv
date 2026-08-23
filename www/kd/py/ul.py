#!/usr/bin/python
from __future__ import print_function

import sys
import os

import cgi
import cgitb


print("Content-Type: text/plain", end="\r\n\r\n");

form = cgi.FieldStorage()

print("Python : UPLOAD!")

if "file" in form:
    file_item = form["file"]
    if file_item.filename:
        print("file name", file_item.filename)
        file_name = file_item.filename
        file_path = './upload-py-' + file_name
        with open(file_path, 'wb') as file:
            file.write(file_item.file.read())
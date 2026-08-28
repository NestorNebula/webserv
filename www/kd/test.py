#!/usr/bin/python3

from __future__ import print_function

import sys
import os

import cgi
import cgitb

# print("Content-Type: text/plain", end="\r\n\r\n");

form = cgi.FieldStorage()

g1 = form.getvalue("g1", "g1-default")
g2 = form.getvalue("g2", "g2-default")

p1 = form.getvalue("p1", "p1-default")
p2 = form.getvalue("p2", "p2-default")

print("Content-Type: text/plain\r\n\r\n", end="")
print("Python : hello, world!")

print ("\nGET VARS")
print("g1 :", g1)
print("g2 :", g2)

print ("\nPOST VARS")
print("p1 :", p1)
print("p2 :", p2)

print ("\nENV\n")
for key, val in os.environ.items():
    print(key, "=", val)

# WRITE_FILE
# with open("whereami.py.txt", "a") as f:
#     f.write("Now the file has more content!");
          
if "file" in form:
    file_item = form["file"]
    if file_item.filename:
        print("file name", file_item.filename)
        file_name = file_item.filename
        file_path = './uploads/py-' + file_name
        with open(file_path, 'wb') as file:
            file.write(file_item.file.read())
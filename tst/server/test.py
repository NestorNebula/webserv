#!/usr/bin/python
import sys
import os

# what is CWD .. when scripts are called (!)

import cgi
import cgitb

form = cgi.FieldStorage()

g1 = form.getvalue("g1", "g1-default")
g2 = form.getvalue("g2", "g2-default")

p1 = form.getvalue("p1", "p1-default")
p2 = form.getvalue("p2", "p2-default")

print()
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

if "file" in form:
    file_item = form["file"]
    if file_item.filename:
        print("file name", file_item.filename)
        file_name = file_item.filename
        file_path = './' + file_name
        with open(file_path, 'wb') as file:
            file.write(file_item.file.read())
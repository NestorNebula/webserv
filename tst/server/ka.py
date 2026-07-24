#!/usr/bin/python
from __future__ import print_function

import sys
import os


# what is CWD .. when scripts are called (!)

import cgi
import cgitb


print("Content-Type: text/plain", end="\r\n")
print("Connection: keep-alive", end="\r\n")
print("Content-Length: 17", end="\r\n\r\n")

# print ("I'm not dead yet.", end=""); # ugly
print ("I'm not dead yet.")
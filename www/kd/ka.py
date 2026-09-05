#!/usr/bin/python
from __future__ import print_function

import sys
import os

import cgi
import cgitb

print("Content-Type: text/plain", end="\r\n")
print("Connection: keep-alive", end="\r\n")
print("Content-Length: 222", end="\r\n\r\n")

print ("PYTHON is not dead (yet).", end="")
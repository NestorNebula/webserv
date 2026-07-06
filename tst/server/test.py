#!/usr/bin/python
import sys
import os

# what is CWD .. when scripts are called (!)
# ENV : dir ... from which we launched webserv 
sys.path.append("/home/kdonlon/Documents/Projects/webserv/legacy-cgi-main/");
# Deprecated since version 3.11, removed in version 3.13.
import cgi
import cgitb

form = cgi.FieldStorage()
p1 = form.getvalue("p1", "p1-default")
p2 = form.getvalue("p2", "p2-default")

env = os.environ['CONTENT_LENGTH']
body = "Python : hello, world!\n"

siz = len(p1) + len(p2) + len(body) + len(env)

# print("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n");

# what about the query string (?)

print(p1)
print(p2)
print(body)
print(env)


# KEEP_ALIVE
# print("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: Keep-Alive\r\nContent-Length: 23\r\n\r\nPython : hello, world!\n");

#!/usr/bin/python
import os
# not school -- nice test for error in (cgi) script
import cgi

# export QUERY_STRING="p1=one&p2=two"

form = cgi.FieldStorage()
p1 = form.getvalue("p1", "p1-default")
p2 = form.getvalue("p2", "p2-default")

env = os.environ['CONTENT_LENGTH']
body = "Python : hello, world!\n"

siz = len(p1) + len(p2) + len(body) + len(env)

print("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + str(siz + 4) + "\r\n\r\n");

print(p1)
print(p2)
print(body)
print(env)


# KEEP_ALIVE
# print("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: Keep-Alive\r\nContent-Length: 23\r\n\r\nPython : hello, world!\n");

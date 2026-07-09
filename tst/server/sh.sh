#!/bin/bash

# siege -f staging-urls.txt --internet --verbose --reps=2 --concurrent=255 --no-parser -b
# echo
# exit 0

# KEEP_ALIVE
# siege --delay=0.01 -f staging-urls.txt --internet --verbose --reps=1 --concurrent=80 --no-parser -b --header="Connection:keep-alive"
# echo
# exit 0

# siege --delay=0.1 -f staging-urls.txt --internet --verbose --reps=2 --concurrent=2 --no-parser 
# echo
# exit 0

# curl -X GET http://localhost:8080
# echo
# curl -X GET http://localhost:8081
# echo
# curl -X GET http://localhost:8082
# echo
# exit 0


# curl -X POST http://localhost:8081
# echo
# exit 0


# curl -X GET 'http://localhost:8080/?p1=one&p2=two'
# echo

# (firefox !)
curl -X POST http://localhost:8080 \
	 -d "p1=post-one&p2=post-two"
echo
exit 0

# curl -X POST http://localhost:8080 \
# 	 -d "p1=post-one&p2=post-two"
# echo
# curl -X POST http://localhost:8081 \
# 	 -d "p1=post-one&p2=post-two"
# echo
# curl -X POST http://localhost:8082 \
# 	 -d "p1=post-one&p2=post-two"
# echo
# exit 0

# curl -X POST http://localhost:8081 \
# 	-H "Content-Type: application/x-www-form-urlencoded" \
# 	-d "p1=post-one&p2=post-two"
# echo
# exit 0

	# multipart/form-data
# curl -X POST http://localhost:8081 \
# 	-F p1=post-one \
# 	-F p2=post-two \
# 	-F file=@files/2k_earth_daymap.jpg
# echo
# exit 0

	# -F file=@Kanan.mp3
	
# POST / HTTP/1.1
# Host: localhost:8081
# User-Agent: curl/8.11.1
# Accept: */*
# Content-Length: 14976173
# Content-Type: multipart/form-data; boundary=------------------------pmbnJBZpu2KOjNtCuj9mAu
# Expect: 100-continue ***


# echo
# 	# chunked - needs to be parsed before passing to CGI
# curl -X POST http://localhost:8081 \
# 	-H "Transfer-Encoding: chunked" \
# 	-d @files/2k_earth_daymap.jpg


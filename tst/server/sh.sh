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

# curl -X GET http://127.0.0.1:8080/index.html
# echo
# exit 0
# curl -X GET http://localhost:8081?a1=one -i
# echo
# curl -X POST http://localhost:8082?b2=two -i
# echo
# exit 0

curl -X GET 'http://localhost:8080/?g1=sig_one&g2=sig_two' \
	 -d "p1=post-one&p2=post-two"
echo
exit 0

curl -X POST http://localhost:8081/test.py \
	 -d "p1=post-one&p2=post-two"
echo
exit 0

curl -X POST http://localhost:8082/test.pl \
	 -d "p1=post-one&p2=post-two"
echo
exit 0


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
# 	# -F file=@files/Kanan.mp3
# echo
# exit 0

# POST / HTTP/1.1
# Host: localhost:8081
# User-Agent: curl/8.11.1
# Accept: */*
# Content-Length: 14976173 -- or .. we need to track this .. and (close) input to cgi when done 
# Content-Type: multipart/form-data; boundary=------------------------pmbnJBZpu2KOjNtCuj9mAu
# Expect: 100-continue ***
	# do NOT send Content-Length to (cgi)

    # Chunked transfer encoding allows a server to maintain an HTTP persistent connection for dynamically generated content. In this case, the HTTP Content-Length header cannot be used to delimit the content and the next HTTP request/response, as the content size is not yet known. Chunked encoding has the benefit that it is not necessary to generate the full content before writing the header, as it allows streaming of content as chunks and explicitly signaling the end of the content, making the connection available for the next HTTP request/response.
    # Chunked encoding allows the sender to send additional header fields after the message body. This is important in cases where values of a field cannot be known until the content has been produced, such as when the content of the message must be digitally signed. Without chunked encoding, the sender would have to buffer the content until it was complete in order to calculate a field value and send it before the content.


	# chunked - needs to be parsed before passing to CGI
	# not the same as form (?)
	# "pure" upload .. "PUT"
	# we do not respond to this properly 
	# not actually a (cgi) thing (?)
	# NB : not a FORM
	# Content-Type: application/x-www-form-urlencoded

	# -H "Transfer-Encoding: chunked" \

# NB: (-d) not part of a FORM .. 
# which is what my test.* files are expecting
# just .. data (?)

# "file upload"

# curl -X POST http://localhost:8081 \
# 	-d @files/2k_earth_daymap.jpg
# echo
# exit 0


# Transfer-Encoding: chunked
# Content-Type: multipart/form-data; boundary=------------------------d75ef80967bc104b
# Expect: 100-continue

# without (chunked)
# content-length tells cgi when it has enough
	# -H "Transfer-Encoding: chunked" \

curl -X POST http://localhost:8082 \
	-H "Transfer-Encoding: chunked" \
	-F p1=post-one \
	-F p2=post-two \
	-F file=@files/Kanan.mp3

	# -F file=@files/2k_earth_daymap.jpg
echo
exit 0


#!/bin/bash

if [ "$1" == "s" ]; then
	siege -f staging-urls.sh --internet --verbose --reps=5 --concurrent=10 --no-parser -b
	echo
	exit 0
fi

# curl -X GET http://127.0.0.1:8080/bigaudio.php --output data.mp3
# echo
# exit 0

# curl -X GET http://127.0.0.1:8081/bigimage.php --output data.jpg
# echo
# exit 0

# curl -X GET http://localhost:8081/bigvideo.php --output data.mkv
# echo
# exit 0


# curl -X GET http://localhost:8081/suck.php -i
# echo
# curl -X GET http://localhost:8081/suck.py -i
# echo
# curl -X GET http://localhost:8081/suck.pl -i
# echo
# exit 0


# curl -X GET http://localhost:8081/to.php -i
# echo
# exit 0

curl -X POST http://localhost:8081/ka.php -i \
	 -d "p1=post-one&p2=post-two"
echo
exit 0

# curl -X POST http://localhost:8082/test.pl \
# 	 -d "p1=post-one&p2=post-two"
# echo
# exit 0


# curl -X POST http://localhost:8081/test.pl -i \
# 	-H "Content-Type: application/x-www-form-urlencoded" \
# 	-d "p1=post-one&p2=post-two"
# echo
# exit 0



# POST /test.php HTTP/1.1
# Host: localhost:8081
# User-Agent: curl/8.11.1
# Accept: */*
# Content-Length: 14976177
# Content-Type: multipart/form-data; boundary=------------------------smD1LXy5p8xuKzGBs2H6e1
# Expect: 100-continue

# PHP Warning:  PHP Request Startup: POST Content-Length of 14976177 bytes exceeds the limit of 8388608 bytes in Unknown on line 0

curl -X POST http://localhost:8081/test.php -i \
	-F p1=dash-f-one \
	-F p2=dash-f-two \
	-F file=@files/Kanan.mp3
echo
exit 0



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
# cgi .. not looking for more data
# content-length is STRANGE here 

# curl -X POST http://localhost:8081/test.php -i \
# 	-d @files/2k_earth_daymap.jpg
# echo
# exit 0


# Transfer-Encoding: chunked
# Content-Type: multipart/form-data; boundary=------------------------d75ef80967bc104b
# Expect: 100-continue

# curl -X POST http://localhost:8082/test.php \
# 	-F file=@files/2k_earth_daymap.jpg
# echo
# exit 0

# content-length tells cgi when it has enough
	# -H "Transfer-Encoding: chunked" \

	# -H "Content-Type: application/x-www-form-urlencoded" \
# curl -X POST http://localhost:8082/test.php \
# 	-H "Transfer-Encoding: chunked" \
# 	-F p1=chunked_one \
# 	-F p2=chunked_two \
# 	-F file=@files/2k_earth_daymap.jpg
# echo
# exit 0


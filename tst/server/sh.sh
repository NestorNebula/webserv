#!/bin/bash

tput reset

C=250
R=4

while getopts "c:r:" o; do
    case "${o}" in
        c)	C=${OPTARG} ;;
        r)	R=${OPTARG} ;;
        *)	usage ;;
    esac
done
shift $((OPTIND-1))


if [[ "$1" =~ "s" ]]; then
	siege -f urls/staging-urls.sh --internet --verbose --reps=$R --concurrent=$C --no-parser -b
	echo
fi

if [[ "$1" =~ "b" ]]; then
	siege -f urls/big.sh --internet --verbose --reps=$R --concurrent=$C --no-parser -b
	echo
fi


if [[ "$1" =~ "h" ]]; then
	siege -f urls/html.sh --internet --verbose --reps=$R --concurrent=$C --no-parser -b
	echo
fi

if [[ "$1" =~ "k" ]]; then
	siege -f urls/ka.sh -R ./urls/ka.conf --internet --verbose --reps=$R --concurrent=$C --no-parser -b
	echo
fi

if [[ "$1" =~ "p" ]]; then
	siege -f urls/php.sh --internet --verbose --reps=$R --concurrent=$C  -b
	echo
fi

if [[ "$1" =~ "y" ]]; then
	# siege http://localhost:8082/test.py --internet --verbose --reps=$R --concurrent=$C --no-parser -b
	siege -f urls/python.sh --internet --verbose --reps=$R --concurrent=$C  -b
	echo
fi

if [[ "$1" =~ "l" ]]; then
	# siege http://localhost:8082/test.pl --internet --verbose --reps=$R --concurrent=$C --no-parser -b
	siege -f urls/perl.sh --internet --verbose --reps=$R --concurrent=$C  -b
	echo
fi

if [[ "$1" =~ "f" ]]; then
	siege -f urls/fnf.sh --internet --verbose --reps=$R --concurrent=$C --no-parser -b
	# siege -f fnf.sh -R ~/.siege/ka.conf --internet --verbose --reps=$R --concurrent=$C --no-parser -b
	echo
fi


if [[ "$1" =~ "x" ]]; then
	siege -f urls/exit.sh --internet --verbose --reps=$R --concurrent=$C --no-parser -b
	echo
fi

if [[ "$1" =~ "t" ]]; then
	siege -f urls/stat.sh --internet --verbose --reps=$R --concurrent=$C --no-parser -b
	echo
fi

if [ "$1" == "a" ]; then
	curl -X GET http://127.0.0.1:8082/bigaudio.php --output data.mp3
	echo
	exit 0

	# curl -X GET http://127.0.0.1:8082/files/Kanan.mp3 --output data.mp3
	# echo
	# exit 0
fi

if [ "$1" == "j" ]; then
	curl -X GET http://127.0.0.1:8082/bigimage.php --output data-cgi.jpg
	echo

	# curl -X GET http://127.0.0.1:8082/files/earth.jpg --output data-file.jpg
	# echo
	exit 0
fi

if [ "$1" == "v" ]; then
	curl -X GET http://localhost:8082/bigvideo.php -i --output data.mkv
	echo
	exit 0

	# curl -X GET http://127.0.0.1:8082/files/Black.Mirror.S07E04.mkv --output data.mkv
	# echo
	# exit 0
fi


if [ "$1" == "u" ]; then

	WWW=../../www/kd
	rm -f $WWW/php/upload*
	rm -f $WWW/pl/upload*
	rm -f $WWW/py/upload*
	rm -f $WWW/uploads/*

	ls -l $WWW/p*

	FILES=
	FILES+="tiny.jpg "
	FILES+="mid.jpg "
	FILES+="earth.jpg "
	FILES+="e4.jpg "
	FILES+="Kanan.mp3 "

	for FILE in $FILES; do
# ATTN : Kanan : content-length
# WORK HERE 
# FCGI : not 100%
# END STDIN .. still has (left)

		# curl -X POST http://localhost:8082/php/ul.php -i \
		# 	-F file=@$WWW/files/$FILE
		# echo ; echo ; echo
		# curl -X POST http://localhost:8082/pl/ul.pl -i \
		# 	-F file=@$WWW/files/$FILE
		# echo ; echo ; echo
		# curl -X POST http://localhost:8082/py/ul.py -i \
		# 	-F file=@$WWW/files/$FILE


		# curl -X POST http://localhost:8082/uploads/FILE -i \
		# 	-F file=@$WWW/files/$FILE

		curl -X POST http://localhost:8082/uploads/$FILE -i \
			-H "Content-Type:application/octet-stream" \
			--data-binary @$WWW/files/$FILE
		echo ; echo ; echo
	done

	ls -l $WWW/p*
	ls -l $WWW/uploads

	exit 0
fi


if [ "$1" ]; then
	exit 0
fi



# curl -X GET 'http://localhost:8082/'
# echo
# exit 0


curl -X POST 'http://localhost:8082/test.php?g1=QUERYSHIT' -i \
	-d "p1=post-one&p2=post-two"
echo
exit 0

curl -X POST 'http://localhost:8082/stat.php?g1=QUERYSHIT' -i \
	-d "p1=post-one&p2=post-two"
echo
exit 0
# -H "Content-Type:application/octet-stream" --data-binary @asdf.file http://server:1234/url

curl -X POST http://localhost:8080/uploads/small \
	-H "Content-Type:application/octet-stream" \
	--data-binary @www/files/e4.jpg
echo ; echo ; echo
exit 0
# -F file=@www/files/earth.jpg


# -H "Transfer-Encoding: chunked" 
# curl -i -X POST -F @www/files/earth.jpg http://127.0.0.1:7777/php/ul.php
# echo
# exit 0

# -H "Connection: keep-alive" \

	# -d "p1=post-one&p2=post-two" \
# curl -X POST http://localhost:7777/test.php -i \
# 	-H "Content-Type: application/x-www-form-urlencoded" \
# 	 -H "Transfer-Encoding: chunked" \
# 	-d @wtf.txt
# echo
# exit 0

exit 0

# curl -X GET http://localhost:8081/py/ul.py -i
# echo
# exit 0

# curl -X GET http://localhost:8081/suck.py -i
# echo
# curl -X GET http://localhost:8082/suck.pl -i
# echo
# exit 0


# curl -X GET http://localhost:8081/to.php -i
# echo
# exit 0
# test.php?g1=gee-one&g2=gee-two' -i \
# curl -X GET 'http://localhost:8082/index.html' -i \
# 	 -d "p1=post-one&p2=post-two"
# echo
# exit 0

# Content-Length: 463274
# Content-Type: multipart/form-data; boundary=------------------------86fb49c9ac1e2c93

# --------------------------86fb49c9ac1e2c93
# Content-Disposition: form-data; name="data"; filename="earth.jpg"
# Content-Type: image/jpeg

# ����ExifII�
# curl -i -X POST -H "Content-Type: multipart/form-data" \
# 	 -F "data=@www/files/earth.jpg" http://127.0.0.1:8082/media/1234/uploads
# echo
# exit 0


# content-length tells cgi when it has enough
	# -H "Transfer-Encoding: chunked" \

	# -H "Content-Type: application/x-www-form-urlencoded" \
	# -H "Transfer-Encoding: chunked" \
curl -X POST http://localhost:8082/test.php \
	-F p1=chunked_one \
	-F p2=chunked_two \
	-F file=@www/files/earth.jpg
echo
exit 0







curl -X GET 'http://localhost:8080/contact.html' -i
echo
curl -X GET 'http://localhost:8080/index.html' -i
echo
exit 0

curl -X POST http://localhost:8082/test.pl -i \
	-d "p1=post-one&p2=post-two"
echo
exit 0

curl -X POST http://localhost:8082/test.py -i \
	-d "p1=post-one&p2=post-two"
echo
exit 0



# POST /test.php HTTP/1.1
# Host: localhost:8081
# User-Agent: curl/8.11.1
# Accept: */*
# Content-Length: 14976177
# Content-Type: multipart/form-data; boundary=------------------------smD1LXy5p8xuKzGBs2H6e1
# Expect: 100-continue

# PHP Warning:  PHP Request Startup: POST Content-Length of 14976177 bytes exceeds the limit of 8388608 bytes in Unknown on line 0




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
# 	-d @www/files/earth.jpg
# echo
# exit 0


# Transfer-Encoding: chunked
# Content-Type: multipart/form-data; boundary=------------------------d75ef80967bc104b
# Expect: 100-continue

# curl -X POST http://localhost:8082/test.php \
# 	-F file=@www/files/earth.jpg
# echo
# exit 0




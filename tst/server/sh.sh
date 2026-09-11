#!/bin/bash

tput reset

C=250
R=8

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

if [[ "$1" =~ "n" ]]; then
	siege -f urls/noa.sh --internet --verbose --reps=$R --concurrent=$C --no-parser -b
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
	siege -f urls/php.sh --internet --verbose --reps=$R --concurrent=$C --no-parser -b
	echo
fi

if [[ "$1" =~ "y" ]]; then
	# siege http://localhost:8082/cgi-vars/vars.py --internet --verbose --reps=$R --concurrent=$C --no-parser -b
	siege -f urls/python.sh --internet --verbose --reps=$R --concurrent=$C  -b
	echo
fi

if [[ "$1" =~ "l" ]]; then
	# siege http://localhost:8082/cgi-vars/vars.pl --internet --verbose --reps=$R --concurrent=$C --no-parser -b
	siege -f urls/perl.sh --internet --verbose --reps=$R --concurrent=$C  -b
	echo
fi

if [[ "$1" =~ "f" ]]; then
	siege -f urls/fnf.sh --internet --verbose --reps=$R --concurrent=$C --no-parser -b
	echo
fi

if [[ "$1" =~ "x" ]]; then
	siege -f urls/exit.sh --internet --verbose --reps=$R --concurrent=$C --no-parser -b
	echo
fi





if [ "$1" == "a" ]; then
	curl -X GET http://127.0.0.1:8082/cgi-big/bigaudio.php --output data.mp3
	echo
	exit 0

	# curl -X GET http://127.0.0.1:8082/files/Kanan.mp3 --output data.mp3
	# echo
	# exit 0
fi

if [ "$1" == "j" ]; then
	curl -X GET http://127.0.0.1:8082/cgi-big/bigimage.php --output data-cgi.jpg
	echo

	# curl -X GET http://127.0.0.1:8082/files/earth.jpg --output data-file.jpg
	# echo
	exit 0
fi

if [ "$1" == "v" ]; then
	curl -X GET http://localhost:8082/cgi-big/bigvideo.php -i --output data.mkv
	echo
	exit 0

	# curl -X GET http://127.0.0.1:8082/files/Black.Mirror.S07E04.mkv --output data.mkv
	# echo
	# exit 0
fi


WWW=../../www/kd
if [ "$1" == "u" ]; then

	rm -f $WWW/cgi-uploads/upload*
	rm -f $WWW/uploads/*

	ls -l $WWW/cgi-uploads/upload* 2>/dev/null

	FILES=
	FILES+="tiny.jpg "
	FILES+="mid.jpg "
	FILES+="earth.jpg "
	FILES+="e4.jpg "
	# FILES+="Kanan.mp3 "

	for FILE in $FILES; do

		curl -X POST http://localhost:8081/cgi-uploads/ul.php \
			-F file=@$WWW/files/$FILE
		echo ; echo ; echo
		curl -X POST http://localhost:8081/cgi-uploads/ul.pl \
			-F file=@$WWW/files/$FILE
		echo ; echo ; echo
		curl -X POST http://localhost:8081/cgi-uploads/ul.py \
			-F file=@$WWW/files/$FILE
		echo ; echo ; echo ;

		# curl -X POST http://localhost:8082/uploads/$FILE \
		# 	-F file=@$WWW/files/$FILE
		# echo ; echo ; echo ;

		# curl -X POST http://localhost:8082/uploads/$FILE -i \
		# 	-H "Content-Type:application/octet-stream" \
		# 	-H "Transfer-Encoding: chunked" \
		# 	--data-binary @$WWW/files/$FILE

			# -H "Transfer-Encoding: chunked" \
		# curl -X POST http://localhost:8082/uploads/$FILE -i \
		# 	-H "Content-Type: application/x-www-form-urlencoded" \
		# 	-F file=@$WWW/files/$FILE

		echo ; echo ; echo
	done

	ls -l $WWW/cgi-uploads/upload* 2>/dev/null
	ls -l $WWW/uploads

	exit 0
fi


if [ "$1" ]; then
	exit 0
fi




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

# req   : Sending data to closed request

# curl -X POST http://localhost:8082/cgi-uploads/ul.php \
# 	-H "Content-Type: application/x-www-form-urlencoded" \
# 	-H "Transfer-Encoding: chunked" \
# 	-F file=@$WWW/files/earth.jpg

# # curl -X POST http://localhost:8081/cgi-vars/vars.php -i \
# # 	-d @www/files/earth.jpg
# echo
# exit 0



# My understanding of the bug and the fix is that if the data is chunked and no Content-Length is specified, Apache will add that header before it gets to PHP, so PHP will read the data.

# -H "Transfer-Encoding: chunked"
# curl -i -X POST -F @www/files/earth.jpg http://127.0.0.1:7777/php/ul.php
# echo
# exit 0

# -H "Connection: keep-alive" \

	# -d "p1=post-one&p2=post-two" \
curl -X GET http://localhost:8082/favicon.ico -i --output favi.ico
echo
exit 0

# curl --http1.0 -X POST 'http://localhost:8082/ka.php' -i

# curl -X POST 'http://localhost:8082/cgi-codes/stat.php' -i \
# 	-F "code=1"
# echo


# fastcgi .. error .. returns HTTP HEADE (!)
# could be a pref thing
curl -X POST 'http://localhost:8082/cgi-codes/exit.php' -i \
	-F "code=0"
echo

# echo
# curl -X GET 'http://localhost:8082/cgi-codes/exit.pl' -i
# echo
# curl -X GET 'http://localhost:8082/cgi-codes/exit.py' -i
# echo
exit 0

curl -X GET 'http://localhost:8082/cgi-vars/vars.php?g1=QUERY&g2=both' -i \
	-d "p1=siege-post-one&p2=siege-post-two"
echo
exit 0

curl -X POST 'http://localhost:8082/cgi-codes/stat.php?g1=QUERY' -i \
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


exit 0

# curl -X GET http://localhost:8081/py/ul.py -i
# echo
# exit 0

# curl -X GET http://localhost:8081/timeout.php -i
# echo
# exit 0
# vars/vars.php?g1=gee-one&g2=gee-two' -i \
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
curl -X POST http://localhost:8082/cgi-vars/vars.php \
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

curl -X POST http://localhost:8082/cgi-vars/vars.pl -i \
	-d "p1=post-one&p2=post-two"
echo
exit 0

curl -X POST http://localhost:8082/cgi-vars/vars.py -i \
	-d "p1=post-one&p2=post-two"
echo
exit 0


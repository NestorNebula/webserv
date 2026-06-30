#!/bin/bash

# curl -X GET 'http://localhost:8080/?p1=one&p2=two'
# echo
# exit 0


# curl -X POST http://localhost:8080 \
# 	 -d "p1=one&p2=two"
# echo
# exit 0

# siege --delay=0.01 -f staging-urls.txt --internet --verbose --reps=2 --concurrent=10 --no-parser -b
# echo
# exit 0

# KEEP_ALIVE
# siege --delay=0.01 -f staging-urls.txt --internet --verbose --reps=1 --concurrent=80 --no-parser -b --header="Connection:keep-alive"
# echo
# exit 0

# siege --delay=0.1 -f staging-urls.txt --internet --verbose --reps=4 --concurrent=2 --no-parser 
# echo
# # exit 0

curl -X POST http://localhost:8080
echo

curl -X POST http://localhost:8081
echo
exit 0



curl -X POST http://localhost:8081 \
	-H "Content-Type: application/x-www-form-urlencoded" \
	-d "p1=one&p2=two"
echo

	# multipart/form-data
curl -X POST http://localhost:8081 \
	-F p1=one \
	-F p2=two \
	-F file=@sh.sh

echo
	# chunked
curl -X POST http://localhost:8081 \
	-H "Transfer-Encoding: chunked" \
	-d @sh.sh


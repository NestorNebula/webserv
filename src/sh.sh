#!/bin/bash



# siege --delay=0.1 -f staging-urls.txt --internet --verbose --reps=10 --concurrent=10 --no-parser
# echo
# exit 0

# siege --delay=0.1 -f staging-urls.txt --internet --verbose --reps=4 --concurrent=4 --no-parser 


curl -X POST http://localhost:8080
echo

# curl -X POST http://localhost:8081
# echo

exit 0


curl -X POST http://localhost:8080 \
	 -d "p1=one&p2=two"
echo
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


#!/bin/bash

tput reset

C=250
R=4

function usage()
{
	echo "attack.sh -c CONCURRENT -r REPETITIONS a|b|c|f|h"
	echo
	echo "defaults"
	echo "-c $C"
	echo "-r $R"
	echo
	echo "attack the server with:"
	echo
	echo "siege -f FILE_LIST --internet --verbose --reps=REPETITIONS --concurrent=CONCURRENT --no-parser -b"
	echo
	echo "a) a mix of all files"
	echo "b) large non-cgi files"
	echo "c) cgi files"
	echo "f) file-not-found"
	echo "h) non-cgi files"
	echo "p) php files"

	echo
	exit
}

while getopts "c:r:" o; do
    case "${o}" in
        c)	C=${OPTARG} ;;
        r)	R=${OPTARG} ;;
        *)	usage ;;
    esac
done
shift $((OPTIND-1))

function attack()
{
	siege -f $1 --internet --verbose --reps=$R --concurrent=$C --no-parser -b
}

case "$1" in
	a)	URLS=urls/all.sh ;;
	b)	URLS=urls/big.sh ;;
	c)	URLS=urls/cgi.sh ;;
	f)	URLS=urls/fnf.sh ;;
	h)	URLS=urls/html.sh ;;
	p)	URLS=urls/php.sh ;;
	*)	usage ;;
esac

attack $URLS
exit


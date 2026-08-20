#!/bin/bash

tput reset
if [[ "$1" =~ "r" ]]; then
	make fclean
fi
if [[ "$1" =~ "c" ]]; then
	make fclean
	exit 0
fi

make -j12 || exit 1

if [[ "$1" =~ "x" ]]; then
	exit 0
fi

CONF=../../demo/config.conf
tput reset
if [[ "$1" =~ "v" ]]; then
	valgrind -s --track-fds=yes --leak-check=full --show-leak-kinds=all --track-origins=yes ./test "$CONF" 0
else
	./test "$CONF" "$1"
fi

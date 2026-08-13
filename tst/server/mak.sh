#!/bin/bash

tput reset
if [[ "$1" =~ "r" ]]; then
	make fclean
fi
make -j12 || exit 1

if [[ "$1" =~ "x" ]]; then
	exit 0
fi

tput reset
if [[ "$1" =~ "v" ]]; then
	valgrind -s --track-fds=yes --leak-check=full --show-leak-kinds=all --track-origins=yes ./test 0
else
	./test "$1"
fi

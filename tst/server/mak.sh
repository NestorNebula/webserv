#!/bin/bash

tput reset
# "r"
if [ "$1" == "r" ]; then
	make fclean
fi
make -j12
tput reset
if [ "$1" == "v" ]; then
	valgrind -s --track-fds=yes --leak-check=full --show-leak-kinds=all --track-origins=yes ./test
else
	./test
fi

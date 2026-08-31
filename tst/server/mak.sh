#!/bin/bash

tput reset

cd ../..

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

CONF=www/kd/config.conf
# CONF=www/nh/config.conf
tput reset
if [[ "$1" =~ "v" ]]; then
	valgrind -s --track-fds=yes --leak-check=full --show-leak-kinds=all --track-origins=yes ./webserv "$CONF" 0
elif [[ "$1" =~ "g" ]]; then
	gdb --args ./webserv "$CONF" 0
else
	ARG=
	if [[ "$1" =~ "0" ]]; then
		ARG=0
	elif [[ "$1" =~ "k" ]]; then
		ARG=k
	elif [[ "$1" =~ "a" ]]; then
		ARG=a
	fi

	./webserv "$CONF" $ARG
fi

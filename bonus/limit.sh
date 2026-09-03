#!/bin/bash

PROC=valgrind
PROC=webserv

# PID=$(ps -eo pid,comm | grep webserv | awk '{print $1}')
PID=$(pidof $PROC)

tput reset 


function current()
{
    cat /proc/$PID/limits | grep 'open files'
    exit 0;
}

if [[ -z $1 ]]; then
    current
fi

if [[ "$1" == "cur" ]];then 
    ls /proc/$PID/fd | wc -l
    exit 0
fi

if [[ "$1" == "show" ]];then 
    ls -al /proc/$PID/fd
    exit 0
fi

if ! [[ "$1" =~ ^[0-9]+$ ]]; then
    current
fi

CNT="$1"

# can't make higher (!)

prlimit --pid $PID --nofile=$CNT

current


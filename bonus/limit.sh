#!/bin/bash

tput reset

PROC=webserv

function usage()
{
    echo "limit.sh : NUM | list"
}

function current()
{
    cat /proc/$PID/limits | grep 'open files'
    echo "Currently open : "$(ls /proc/$PID/fd | wc -l)
    exit 0;
}

# PID=$(ps -eo pid,comm | grep webserv | awk '{print $1}')
PID=$(pidof $PROC)

if [[ -z $PID ]]; then
    echo "$PROC : not running"
    PROC=valgrind.bin
    PID=$(pidof $PROC)
    if [[ -z $PID ]]; then
        exit 0
    fi
    echo "using : $PROC"
fi

if [[ -z $1 ]]; then
    usage
    echo
    current
fi

if [[ "$1" == "list" ]];then
    # ls -lG --hyperlink=always /proc/$PID/fd
    # find /proc/$PID/fd -type l -ls
    # ls -l $(find /proc/$PID/fd -type l)
    stat --format=%N $(find /proc/$PID/fd -type l)
    # stat  $(find /proc/$PID/fd -type l)
    # find /proc/$PID/fd -type l
    exit 0
fi

if ! [[ "$1" =~ ^[0-9]+$ ]]; then
    usage
    echo
    current
fi

# NB : can't increase without (sudo)

CNT="$1"
prlimit --pid $PID --nofile=$CNT

current


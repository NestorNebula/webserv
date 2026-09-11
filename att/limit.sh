#!/bin/bash

tput reset

PROC=webserv

function usage()
{
    echo "limit.sh : NUM | list"
	echo
	echo "Use this script to limit the number of fds allowed for webserv"
    echo
}

PID=
function get_pid()
{
    # PID=$(ps -eo pid,comm | grep webserv | awk '{print $1}')
    if [[ $PID ]]; then
        return

    fi
    echo "# pidof $PROC"
    pidof $PROC
    echo
    PID=$(pidof $PROC)

    if [[ -z $PID ]]; then
        echo "$PROC : not running"
        exit
    fi

}

function current()
{
    get_pid;

    echo "# cat /proc/$PID/limits | grep 'open files'"
    cat /proc/$PID/limits | grep 'open files'
    echo
    echo "# ls /proc/$PID/fd | wc -l"
    echo "Currently open            "$(ls /proc/$PID/fd | wc -l)
    exit 0;
}


if [[ -z $1 ]]; then
    usage
    current
    exit
fi

get_pid

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
fi

# NB : can't increase without (sudo)

CNT="$1"
echo "# prlimit --pid $PID --nofile=$CNT"
prlimit --pid $PID --nofile=$CNT
echo

current


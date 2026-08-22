#!/bin/bash

tput reset


usage ()
{
    echo "usage: ./pycgi.sh install|clean"
    exit 0
}

[ -z "$1" ] && usage

PYCGI_TARBALL=pycgi.tar.gz
PYCGI_FOLDER=legacy-cgi-2.6

# https://pypi.org/project/legacy-cgi/
# https://pypi.org/project/legacy-cgi/2.6/#files

if [ "$1" == "install" ]; then
    if [ ! -f $PYCGI_TARBALL ]; then
        curl https://files.pythonhosted.org/packages/8c/de/d5385d8e6f37ac1f19d9839eaab2f10bd7062ad33b7d23075553baf4c1d2/legacy-cgi-2.6.tar.gz -o $PYCGI_TARBALL
    fi
    if [ ! -d $PYCGI_FOLDER ]; then
        tar xvf $PYCGI_TARBALL
    fi
    exit 0
fi

if [ "$1" == "clean" ]; then
    rm -f $PYCGI_TARBALL
    rm -fr $PYCGI_FOLDER
    exit 0
fi

usage



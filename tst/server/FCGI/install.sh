#!/bin/bash

tput reset

if [ "$1" == "clean" ]; then
    echo "cleaning ..."
    rm -fr bin include lib share ltmain.sh
    rm -fr ./src/fcgi2-2.4.5
    echo "done"
    exit 0
fi


PHP_FPM_BIN=/usr/sbin/php-fpm7.4
# PHP_FPM_BIN=/usr/bin/php-fpm

if [ "$1" == "run" ]; then

systemctl --user daemon-reload 
systemctl --user start php-fpm.service

# why can't I ctrl-c out of this (?)
# /usr/sbin/php-fpm7.4 --nodaemonize --fpm-config /media/kdonlon/data/Documents/42/webserv/git/tst/server/FCGI/.php-fpm/php-fpm.conf
    exit 0
fi
SRC_DIR=./src
TGT_DIR=$(pwd)

FPM_DIR=$TGT_DIR/.php-fpm

if [ "$1" == "conf" ]; then
#     cat << EOF > ~/.config/systemd/user/php-fpm.service
# [Unit]
# Description=PHP FastCGI process manager
# After=local-fs.target network.target
# # nginx.service

# [Service]
# PIDFile=$FPM_DIR/PID
# ExecStart=$PHP_FPM_BIN --nodaemonize \
# --fpm-config $FPM_DIR/php-fpm.conf

# Type=simple

# [Install]
# WantedBy=multi-user.target
# EOF

    mkdir -p $FPM_DIR
    mkdir -p $FPM_DIR/php-fpm.d

    sed -e "s#FPM_DIR#$FPM_DIR#" $SRC_DIR/php-fpm.conf > $FPM_DIR/php-fpm.conf
    sed -e "s#FPM_DIR#$FPM_DIR#" $SRC_DIR/www.conf > $FPM_DIR/php-fpm.d/www.conf
    exit 0
fi


cd $SRC_DIR
if ! test -d fcgi2-2.4.5; then
    echo -n "decompressing tarball ... "
    tar xf libfcgi_2.4.5.orig.tar.gz
    echo "done"
    echo
fi

cd fcgi2-2.4.5
if ! test -f ltmain.sh; then
    echo "run : autogen.sh (fail)"
    ./autogen.sh > /dev/null
    echo "run : autogen.sh (success)"
    ./autogen.sh
    echo
    echo
fi


echo "config ; make ; make install"
echo $TGT_DIR
echo
./configure --prefix=$TGT_DIR && make && make install


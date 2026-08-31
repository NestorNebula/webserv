#!/bin/bash

tput reset

usage ()
{
    echo "usage: ./fcgi.sh conf|start|stop|cmd|clean"
    exit 0
}

[ -z "$1" ] && usage

TGT_DIR=$(pwd)
FPM_DIR=$TGT_DIR/.php-fpm


PHP_FPM_BIN=$(which php-fpm)
if [[ -z $PHP_FPM_BIN ]]; then
    PHP_FPM_BIN=/usr/sbin/php-fpm7.4
    # echo "php-fpm : not found"
    # exit 0
fi


SRC_DIR=./src
if [ "$1" == "conf" ]; then
    OFILE=~/.config/systemd/user/php-fpm.service

    echo "Generating and installing :"
    echo "$OFILE"
    cat << EOF > ~/.config/systemd/user/php-fpm.service
[Unit]
Description=PHP FastCGI process manager
After=local-fs.target network.target
# nginx.service

[Service]
PIDFile=$FPM_DIR/PID
ExecStart=$PHP_FPM_BIN --nodaemonize \
--fpm-config $FPM_DIR/php-fpm.conf

Type=simple

[Install]
WantedBy=multi-user.target
EOF
    echo

    mkdir -p $FPM_DIR
    mkdir -p $FPM_DIR/php-fpm.d

    OFILE=$FPM_DIR/php-fpm.conf

    echo "Generating and installing :"
    echo "$OFILE"
    sed -e "s#FPM_DIR#$FPM_DIR#" $SRC_DIR/php-fpm.conf.src > $OFILE
    echo

    OFILE=$FPM_DIR/php-fpm.d/www.conf

    echo "Generating and installing :"
    echo "$OFILE"
    sed -e "s#FPM_DIR#$FPM_DIR#" $SRC_DIR/www.conf.src > $OFILE
    echo
    exit 0
fi


if [ "$1" == "start" ]; then
    echo "starting : php-fpm as systemd --user service"
    systemctl --user daemon-reload 
    systemctl --user start php-fpm.service
    ./pwatch
    exit 0
fi

if [ "$1" == "stop" ]; then
    echo "stopping : php-fpm as systemd --user service"
    systemctl --user stop php-fpm.service
    ./pwatch
    exit 0
fi

if [ "$1" == "cmd" ]; then
    echo "Execute the following command to start php-fpm"
    echo
    echo $PHP_FPM_BIN --nodaemonize --fpm-config $FPM_DIR/php-fpm.conf
    echo
    exit 0
fi

if [ "$1" == "clean" ]; then
    systemctl --user stop php-fpm.service
    rm -f ~/.config/systemd/user/php-fpm.service
    rm -fr $FPM_DIR
    exit 0
fi

usage
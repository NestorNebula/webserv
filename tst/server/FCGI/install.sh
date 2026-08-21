#!/bin/bash

tput reset

TGT_DIR=$(pwd)

FPM_DIR=$TGT_DIR/.php-fpm

# /usr/bin/php-fpm -v

PHP_FPM_BIN=$(which php-fpm)

if [[ -z $PHP_FPM_BIN ]]; then
    PHP_FPM_BIN=/usr/sbin/php-fpm7.4
    # echo "(php-fpm) not found"
    # echo "using : $PHP_FPM_BIN"
fi


if [ "$1" == "conf" ]; then
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

    mkdir -p $FPM_DIR
    mkdir -p $FPM_DIR/php-fpm.d

    sed -e "s#FPM_DIR#$FPM_DIR#" php-fpm.conf.src > $FPM_DIR/php-fpm.conf
    sed -e "s#FPM_DIR#$FPM_DIR#" www.conf.src > $FPM_DIR/php-fpm.d/www.conf
    exit 0
fi



if [ "$1" == "start" ]; then
    systemctl --user daemon-reload 
    systemctl --user start php-fpm.service
    exit 0
fi
if [ "$1" == "stop" ]; then
    systemctl --user stop php-fpm.service
    exit 0
fi

if [ "$1" == "run" ]; then
    echo $PHP_FPM_BIN --nodaemonize --fpm-config $FPM_DIR/php-fpm.conf
    exit 0
fi


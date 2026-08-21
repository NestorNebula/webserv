#!/bin/bash

tput reset

TGT_DIR=$(pwd)

# pycgi.sh : install, clean

if [ "$1" == "pycgi" ]; then
# https://pypi.org/project/legacy-cgi/
# https://pypi.org/project/legacy-cgi/2.6/#files

    PYCGI_TARBALL=pycgi.tar.gz
    PYCGI_FOLDER=legacy-cgi-2.6
    if [ "$2" == "clean" ]; then
        rm -f $PYCGI_TARBALL
        rm -fr $PYCGI_FOLDER
        exit 0
    fi
    if [ ! -f $PYCGI_TARBALL ]; then
        curl https://files.pythonhosted.org/packages/8c/de/d5385d8e6f37ac1f19d9839eaab2f10bd7062ad33b7d23075553baf4c1d2/legacy-cgi-2.6.tar.gz -o $PYCGI_TARBALL
    fi
    if [ ! -d $PYCGI_FOLDER ]; then
        tar xvf $PYCGI_TARBALL # -C folder --strip-components=1
    fi
    exit 0
fi



FPM_DIR=$TGT_DIR/.php-fpm

PHP_FPM_BIN=$(which php-fpm)
if [[ -z $PHP_FPM_BIN ]]; then
    PHP_FPM_BIN=/usr/sbin/php-fpm7.4
    # echo "(php-fpm) not found"
    # echo "using : $PHP_FPM_BIN"
fi

# VALIDATE : $PHP_FPM_BIN

if [ "$1" == "clean" ]; then
    rm -f ~/.config/systemd/user/php-fpm.service
    rm -f $FPM_DIR/php-fpm.conf
    rm -f $FPM_DIR/php-fpm.d/www.conf
    exit 0
fi


SRC_DIR=./src
if [ "$1" == "conf" ]; then
    echo "Generating and installing php-fpm.service file"
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

    echo "Generating and installing php-fpm.conf"
    sed -e "s#FPM_DIR#$FPM_DIR#" $SRC_DIR/php-fpm.conf.src > $FPM_DIR/php-fpm.conf

    echo "Generating and installing www.conf"
    sed -e "s#FPM_DIR#$FPM_DIR#" $SRC_DIR/www.conf.src > $FPM_DIR/php-fpm.d/www.conf
    exit 0
fi

# php-fpm.sh : conf, start, stop, cmd, clean
if [ "$1" == "start" ]; then
    systemctl --user daemon-reload 
    systemctl --user start php-fpm.service
    exit 0
fi
if [ "$1" == "stop" ]; then
    systemctl --user stop php-fpm.service
    exit 0
fi

if [ "$1" == "cmd" ]; then
    echo $PHP_FPM_BIN --nodaemonize --fpm-config $FPM_DIR/php-fpm.conf
    exit 0
fi


#!/bin/bash

tput reset

if [ "$1" == "clean" ]; then
    echo "cleaning ..."
    rm -fr bin include lib share ltmain.sh
    rm -fr ./src/fcgi2-2.4.5
    echo "done"
    exit 0
fi


SRC_DIR=./src
TGT_DIR=$(pwd)

FPM_DIR=$TGT_DIR/.php-fpm

PHP_FPM_BIN=/usr/sbin/php-fpm7.4
# PHP_FPM_BIN=/usr/bin/php-fpm
# /usr/bin/php-fpm
# sometthing in MY conf .. does not error as nicely
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

    sed -e "s#FPM_DIR#$FPM_DIR#" $SRC_DIR/php-fpm.conf > $FPM_DIR/php-fpm.conf
    sed -e "s#FPM_DIR#$FPM_DIR#" $SRC_DIR/www.conf > $FPM_DIR/php-fpm.d/www.conf
    exit 0
fi




cd $SRC_DIR
echo "tar ..."
tar xvf libfcgi_2.4.5.orig.tar.gz
echo
echo

cd fcgi2-2.4.5
echo "autogen.sh"
./autogen.sh > /dev/null
./autogen.sh
echo
echo


echo $TGT_DIR
echo "configure"
./configure --prefix=$TGT_DIR && make && make install


systemctl --user daemon-reload 
systemctl --user start php-fpm.service


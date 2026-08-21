#!/bin/bash

tput reset

nc -C  localhost 8082 << EOF
GET /this HTTP/1.1
Host: localhost
Content-Length:22

EOF

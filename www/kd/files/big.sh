#!/bin/bash

val="words"
siz=40000000
out="./big.txt"

bash -c "printf ${val}%.0s {1..${siz}} > $out"
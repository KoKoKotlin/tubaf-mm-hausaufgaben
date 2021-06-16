#!/bin/sh

cd img
for i in 50 60 70 80 81 82 83 84 85 86 87 88 89 90 99;
do
    echo "qualitiy factor $i: \n"
    ./../dct -q $i -c sails.bmp g.bmp "geil$i.bin"
    echo "\n"
    ./../dct -q $i -d "geil$i.bin" "sails$i.bmp"
    echo "\n"
done
#!/bin/sh

md5sum samples/picture.bmp && ./honkpack < samples/picture.bmp | ./honkpack -d | md5sum
md5sum samples/text.txt && ./honkpack < samples/text.txt | ./honkpack -d | md5sum
md5sum samples/document.pdf && ./honkpack < samples/document.pdf | ./honkpack -d | md5sum

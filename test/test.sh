#!/bin/bash
SOURCE=$NAME.c
OUT=$NAME.out
rm "$OUT" 2> /dev/null

gcc -I"../inc" -O0 -g3 -Wall -Wextra -Winline -std=c99 $SOURCE $C_SRC -o "$OUT"

if [ "$1" = "-m" ]; then
    valgrind --tool=memcheck ./$OUT
else
    ./$OUT
    rm "$OUT" 2> /dev/null
fi

#!/bin/bash
SOURCE=$NAME/$NAME.c
OUT=$NAME.out
rm "$OUT" 2> /dev/null

gcc -I"../inc" -O0 -g3 -Wall -Wextra -Winline -std=gnu99 $SOURCE $C_SRC -o "$OUT"

if [ "$1" = "-m" ]; then
    valgrind --tool=memcheck ./$OUT
else
    ./$OUT
    RET=$?
    rm "$OUT" 2> /dev/null
    exit $RET
fi

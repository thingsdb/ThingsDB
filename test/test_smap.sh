#!/bin/bash
OUT=test_smap.out

rm "$OUT" 2> /dev/null

gcc -I"../inc" -O0 -g3 -Wall -Wextra -Winline -std=c99 test_smap.c ../src/smap/smap.c -o "$OUT"

if [ "$1" = "-m" ]; then
    valgrind --tool=memcheck ./test_smap.out
else
    ./test_smap.out
    rm "$OUT" 2> /dev/null
fi


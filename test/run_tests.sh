#!/bin/bash
RET=0

do_test () {
    ./$1/$1.sh
    rc=$?; if [[ $rc != 0 ]]; then RET=$((RET+1)); fi
}

do_test test_guid
do_test test_link
do_test test_lookup
do_test test_queue
do_test test_smap
do_test test_vec

exit $RET
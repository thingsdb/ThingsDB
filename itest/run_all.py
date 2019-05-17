#!/usr/bin/env python
from lib import run_test

from test_collection_functions import TestCollectionFunctions
from test_gc import TestGC
from test_multi_node import TestMultiNode
from test_user_access import TestUserAccess


if __name__ == '__main__':
    run_test(TestCollectionFunctions())
    run_test(TestGC())
    run_test(TestMultiNode())
    run_test(TestUserAccess())

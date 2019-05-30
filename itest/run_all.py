#!/usr/bin/env python
from lib import run_test

from test_collection_functions import TestCollectionFunctions
from test_gc import TestGC
from test_multi_node import TestMultiNode
from test_node_functions import TestNodeFunctions
from test_nodes import TestNodes
from test_thingsdb_functions import TestThingsDBFunctions
from test_types import TestTypes
from test_user_access import TestUserAccess

if __name__ == '__main__':
    run_test(TestCollectionFunctions())
    run_test(TestGC())
    run_test(TestMultiNode())
    run_test(TestNodeFunctions())
    run_test(TestNodes())
    run_test(TestThingsDBFunctions())
    run_test(TestTypes())
    run_test(TestUserAccess())

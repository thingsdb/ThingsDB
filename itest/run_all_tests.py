#!/usr/bin/env python
from lib import run_test

from test_collection_functions import TestCollectionFunctions
from test_gc import TestGC
from test_index_slice import TestIndexSlice
from test_multi_node import TestMultiNode
from test_nested import TestNested
from test_node_functions import TestNodeFunctions
from test_nodes import TestNodes
from test_operators import TestOperators
from test_procedures import TestProcedures
from test_scopes import TestScopes
from test_thingsdb_functions import TestThingsDBFunctions
from test_types import TestTypes
from test_user_access import TestUserAccess
from test_variable import TestVariable

if __name__ == '__main__':
    run_test(TestCollectionFunctions())
    run_test(TestGC())
    run_test(TestIndexSlice())
    run_test(TestMultiNode())
    run_test(TestNested())
    run_test(TestNodeFunctions())
    run_test(TestNodes())
    run_test(TestOperators())
    run_test(TestProcedures())
    run_test(TestScopes())
    run_test(TestThingsDBFunctions())
    run_test(TestTypes())
    run_test(TestUserAccess())
    run_test(TestVariable())

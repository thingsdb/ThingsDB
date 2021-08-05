#!/usr/bin/env python
import argparse
from lib import run_test, vars

from test_advanced import TestAdvanced
from test_arguments import TestArguments
from test_backup import TestBackup
from test_collection_functions import TestCollectionFunctions
from test_datetime import TestDatetime
from test_dict import TestDict
from test_doc_url import TestDocUrl
from test_enum import TestEnum
from test_events import TestEvents
from test_future import TestFuture
from test_gc import TestGC
from test_http_api import TestHTTPAPI
from test_index_slice import TestIndexSlice
from test_modules import TestModules
from test_multi_node import TestMultiNode
from test_nested import TestNested
from test_node_functions import TestNodeFunctions
from test_nodes import TestNodes
from test_operators import TestOperators
from test_procedures import TestProcedures
from test_relations import TestRelations
from test_room import TestRoom
from test_scopes import TestScopes
from test_syntax import TestSyntax
from test_thingsdb_functions import TestThingsDBFunctions
from test_timers import TestTimers
from test_type import TestType
from test_types import TestTypes
from test_user_access import TestUserAccess
from test_variable import TestVariable
from test_wrap import TestWrap


def no_mem_test(test_class):
    tmp = vars.THINGSDB_MEMCHECK.copy()
    vars.THINGSDB_MEMCHECK.clear()
    run_test(test_class())
    vars.THINGSDB_MEMCHECK[:] = tmp


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--skip-doc-test',
        action='store_true',
        help='skip documentation testing')
    args = parser.parse_args()

    run_test(TestAdvanced())
    run_test(TestArguments())
    run_test(TestBackup())
    run_test(TestCollectionFunctions())
    run_test(TestDatetime())
    run_test(TestDict())
    if args.skip_doc_test is False:
        run_test(TestDocUrl())
    run_test(TestEnum())
    run_test(TestEvents())
    run_test(TestFuture())
    run_test(TestGC())
    run_test(TestHTTPAPI())
    run_test(TestIndexSlice())
    run_test(TestModules())
    run_test(TestMultiNode())
    run_test(TestNested())
    run_test(TestNodeFunctions())
    run_test(TestNodes())
    run_test(TestOperators())
    run_test(TestProcedures())
    run_test(TestRelations())
    run_test(TestRoom())
    run_test(TestScopes())
    run_test(TestSyntax())
    run_test(TestThingsDBFunctions())
    run_test(TestTimers())
    run_test(TestType())
    run_test(TestTypes())
    run_test(TestUserAccess())
    run_test(TestVariable())
    run_test(TestWrap())

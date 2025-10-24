#!/usr/bin/env python
import argparse
from lib import run_test, vars

from test_advanced import TestAdvanced
from test_arguments import TestArguments
from test_backup import TestBackup
from test_changes import TestChanges
from test_collection_functions import TestCollectionFunctions
from test_datetime import TestDatetime
from test_dict import TestDict
from test_doc_url import TestDocUrl
from test_enum import TestEnum
from test_future import TestFuture
from test_gc import TestGC
from test_http_api import TestHTTPAPI
from test_import import TestImport
from test_index_slice import TestIndexSlice
from test_math import TestMath
from test_modules import TestModules
from test_multi_node import TestMultiNode
from test_nested import TestNested
from test_node_functions import TestNodeFunctions
from test_nodes import TestNodes
from test_operators import TestOperators
from test_procedures import TestProcedures
from test_relations import TestRelations
from test_restriction import TestRestriction
from test_room import TestRoom
from test_room_wss import TestRoomWSS
from test_scopes import TestScopes
from test_statements import TestStatements
from test_syntax import TestSyntax
from test_tasks import TestTasks
from test_thingsdb_functions import TestThingsDBFunctions
from test_type import TestType
from test_types import TestTypes
from test_user_access import TestUserAccess
from test_variable import TestVariable
from test_whitelist import TestWhitelist
from test_wrap import TestWrap
from test_ws import TestWS
from test_wss import TestWSS


_hide_version = False


def hide_version() -> bool:
    global _hide_version
    current = _hide_version
    _hide_version = True
    return current


def no_mem_test(test_class):
    tmp = vars.THINGSDB_MEMCHECK.copy()
    vars.THINGSDB_MEMCHECK.clear()
    run_test(test_class(), hide_version=hide_version())
    vars.THINGSDB_MEMCHECK[:] = tmp


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--doc-test',
        action='store_true',
        help='include documentation testing')
    parser.add_argument(
        '--doc-modules',
        action='store_true',
        help='include modules testing')
    args = parser.parse_args()

    run_test(TestAdvanced(), hide_version=hide_version())
    run_test(TestArguments(), hide_version=hide_version())
    run_test(TestBackup(), hide_version=hide_version())
    run_test(TestChanges(), hide_version=hide_version())
    run_test(TestCollectionFunctions(), hide_version=hide_version())
    run_test(TestDatetime(), hide_version=hide_version())
    run_test(TestDict(), hide_version=hide_version())
    if args.doc_test is True:
        run_test(TestDocUrl(), hide_version=hide_version())
    run_test(TestEnum(), hide_version=hide_version())
    run_test(TestFuture(), hide_version=hide_version())
    run_test(TestGC(), hide_version=hide_version())
    run_test(TestHTTPAPI(), hide_version=hide_version())
    run_test(TestImport(), hide_version=hide_version())
    run_test(TestIndexSlice(), hide_version=hide_version())
    run_test(TestMath(), hide_version=hide_version())
    if args.doc_modules is True:
        run_test(TestModules(), hide_version=hide_version())
    run_test(TestMultiNode(), hide_version=hide_version())
    run_test(TestNested(), hide_version=hide_version())
    run_test(TestNodeFunctions(), hide_version=hide_version())
    run_test(TestNodes(), hide_version=hide_version())
    run_test(TestOperators(), hide_version=hide_version())
    run_test(TestProcedures(), hide_version=hide_version())
    run_test(TestRelations(), hide_version=hide_version())
    run_test(TestRestriction(), hide_version=hide_version())
    run_test(TestRoom(), hide_version=hide_version())
    run_test(TestRoomWSS(), hide_version=hide_version())
    run_test(TestScopes(), hide_version=hide_version())
    run_test(TestStatements(), hide_version=hide_version())
    run_test(TestSyntax(), hide_version=hide_version())
    run_test(TestTasks(), hide_version=hide_version())
    run_test(TestThingsDBFunctions(), hide_version=hide_version())
    run_test(TestType(), hide_version=hide_version())
    run_test(TestTypes(), hide_version=hide_version())
    run_test(TestUserAccess(), hide_version=hide_version())
    run_test(TestVariable(), hide_version=hide_version())
    run_test(TestWhitelist(), hide_version=hide_version())
    run_test(TestWS(), hide_version=hide_version())
    run_test(TestWSS(), hide_version=hide_version())
    run_test(TestWrap(), hide_version=hide_version())

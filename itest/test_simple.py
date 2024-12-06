#!/usr/bin/env python
import asyncio
import pickle
import time
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client
from thingsdb.exceptions import AssertionError
from thingsdb.exceptions import ValueError
from thingsdb.exceptions import TypeError
from thingsdb.exceptions import NumArgumentsError
from thingsdb.exceptions import BadDataError
from thingsdb.exceptions import LookupError
from thingsdb.exceptions import OverflowError
from thingsdb.exceptions import ZeroDivisionError
from thingsdb.exceptions import OperationError


class TestSimple(TestBase):

    title = 'Test simple run'

    @default_test_setup(num_nodes=3, seed=1, threshold_full_storage=10)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_hello_world(self, client):
        await client.query('"Hello world!"')

    async def test_names_loading(self, client):
        q = client.query
        # Force a few new keys. (issue #394)
        # Before 1.7.0 ThingsDB did not reload existing names as names, but
        # they were loaded as strings instead. This is not an issue but it is
        # preferred to load them back as names. This checks verifies that
        # names are really restored as cached names, not strings.
        await q("""//ti
            .my_test1 = 1;
            .my_test2 = 2;
            .a = .keys();
            .del('my_test1');
            .del('my_test2');
        """)
        n1 = await q('node_info().load().cached_names;', scope='/n/0')
        await self.node0.shutdown()
        await self.node0.run()
        n2 = await q('node_info().load().cached_names;', scope='/n/0')

        self.assertEqual(n1, n2)


if __name__ == '__main__':
    run_test(TestSimple())

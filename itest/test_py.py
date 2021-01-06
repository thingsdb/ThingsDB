#!/usr/bin/env python
import asyncio
import pickle
import time
from lib import run_test, vars
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
from thingsdb.exceptions import ForbiddenError


class TestPy(TestBase):

    title = 'Test python integration'

    @default_test_setup(num_nodes=1, seed=1, threshold_full_storage=10)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_py(self, client):
        # asyncio.ensure_future(client.query(r'''
        #     future('py');
        # '''))
        await client.query(r'''
            future('py');
        ''')


if __name__ == '__main__':
    # This test does not work with valgrind because of errors when loading
    # Python.
    vars.THINGSDB_MEMCHECK.clear()
    run_test(TestPy())

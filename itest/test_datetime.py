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


class TestDatetime(TestBase):

    title = 'Test datetime (and timeval)'

    @default_test_setup(num_nodes=1, seed=1, threshold_full_storage=10)
    async def run(self):

        self.node0.version()

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_datetime_now(self, client):
        res = await client.query(r'''
            // modulo 2 to prevent a near miss in seconds
            [
                int(datetime()) % 2,
                int(now()) % 2
            ];
        ''')
        self.assertEqual(res[0], res[1])

    async def test_datetime_err(self, client):
        with self.assertRaisesRegex(
                ValueError,
                'name `timeval` is reserved'):
            await client.query('new_type("timeval");')
        with self.assertRaisesRegex(
                ValueError,
                'name `datetime` is reserved'):
            await client.query('set_type("datetime", {});')


if __name__ == '__main__':
    run_test(TestDatetime())

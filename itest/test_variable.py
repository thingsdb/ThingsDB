#!/usr/bin/env python
import asyncio
import pickle
import time
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client
from thingsdb.exceptions import AssertionError
from thingsdb.exceptions import BadDataError
from thingsdb.exceptions import LookupError
from thingsdb.exceptions import OverflowError


class TestVariable(TestBase):

    title = 'Test variable'

    @default_test_setup(num_nodes=1, seed=1)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.use('stuff')

        self.assertEqual(await client.query('a=1;'), 1)
        self.assertEqual(await client.query('a=1; a;'), 1)
        self.assertEqual(await client.query('a=1; a=2; a;'), 2)
        self.assertEqual(await client.query('a=1; a=2; a+=a;'), 4)
        self.assertEqual(await client.query('a=1; a=2; a+=a; a;'), 4)

        client.close()
        await client.wait_closed()


if __name__ == '__main__':
    run_test(TestVariable())

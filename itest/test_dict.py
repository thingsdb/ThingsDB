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
from thingsdb.exceptions import ForbiddenError


class TestDict(TestBase):

    title = 'Test thing as dictionary '

    @default_test_setup(num_nodes=2, seed=1, threshold_full_storage=10)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        # add another node otherwise backups are not possible
        # await self.node1.join_until_ready(client)

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_functions(self, client):
        res = await client.query(r'''
            .dict = {};
            .dict["this is a key"] = 42;
            .dict.set("", "empty key");
            .dict.set("a", 1);
            .dict.set("b", 2);
            .dict.filter(||true);
        ''')
        self.assertEqual(res, {
            "this is a key": 42,
            "": "empty key",
            "a": 1,
            "b": 2,
        })
        res = await client.query(r'''
            d = thing(.dict);
        ''')


if __name__ == '__main__':
    run_test(TestDict())

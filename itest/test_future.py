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


class TestFuture(TestBase):

    title = 'Test futures'

    @default_test_setup(num_nodes=1, seed=1, threshold_full_storage=10)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def _OFF_test_recursion(self, client):
        with self.assertRaisesRegex(
                OperationError,
                r'maximum recursion depth exceeded'):
            await client.query(r'''
                fut = || {
                    future(nil, fut).then(|fut| {
                        fut();
                    });
                };
                fut();
            ''')

    async def _OFF_test_future_gc(self, client):
        await client.query(r'''
            y = {};
            y.y = y;
            future(nil, y).then(|y| {
               x = {};
               x.x = x;
            });
            y = nil;
            "done";
        ''')

    async def test_append_results(self, client):
        res = await client.query(r'''
            ret = [];
            future(nil, "test1", ret).then(|res, ret| {
                ret.push(res);
            });
            future(nil, "test2", ret).then(|res, ret| {
                ret.push(res);
            });

            ret;
        ''')
        self.assertEqual(sorted(res), ['test1', 'test2'])


if __name__ == '__main__':
    run_test(TestFuture())

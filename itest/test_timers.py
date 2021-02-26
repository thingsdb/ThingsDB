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


class TestTimers(TestBase):

    title = 'Test timers'

    @default_test_setup(num_nodes=3, seed=1, threshold_full_storage=10)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_new_timer(self, client):
        with self.assertRaisesRegex(
                LookupError,
                r'function `new_timer` is undefined in the `@node` scope; '
                r'you might want to query the `@thingsdb` or '
                r'a `@collection` scope\?'):
            await client.query('new_timer();', scope='/n')

        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `new_timer`'):
            await client.query('nil.new_timer();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `new_timer` requires at least 2 arguments '
                'but 0 were given'):
            await client.query('new_timer();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `new_timer` takes at most 5 arguments '
                'but 6 were given'):
            await client.query('new_timer(nil, now(), nil, ||nil, [], nil);')

        await client.query(r'''
            .x = 8;
            new_timer(
                nil,
                datetime().move('seconds', 2),
                nil,
                |x| {.x = x},
                [42]
            );
        ''')

        self.assertEqual(await client.query('.x'), 8)
        await asyncio.sleep(20)
        self.assertEqual(await client.query('.x'), 42)

        await client.query(r'''
            .x = 8;
            new_timer(
                datetime().move('seconds', 2),
                |x| {.x = x},
                [42]
            );
        ''')

        self.assertEqual(await client.query('.x'), 8)
        await asyncio.sleep(20)
        self.assertEqual(await client.query('.x'), 42)



if __name__ == '__main__':
    run_test(TestTimers())

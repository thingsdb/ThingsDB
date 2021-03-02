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

    @default_test_setup(num_nodes=1, seed=1, threshold_full_storage=10)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        # add another node otherwise backups are not possible
        if hasattr(self, 'node1'):
            await self.node1.join_until_ready(client)

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def _OFF_test_new_timer(self, client):
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
                'function `new_timer` takes at most 4 arguments '
                'but 5 were given'):
            await client.query('new_timer(datetime(), nil, ||nil, [], nil);')

        with self.assertRaisesRegex(
                ValueError,
                r'repeat value must be at least 30 seconds '
                r'or disabled \(nil\)'):
            await client.query('new_timer(datetime(), 20, ||nil);')

        await client.query(r'''
            new_timer(
                datetime(),
                30,
                |x| {
                    .r = x;
                    set_timer_args([x+1]);
                },
                [1]
            );
        ''')

        await client.query(r'''
            .x = 8;
            new_timer(
                datetime().move('seconds', 2),
                nil,
                |x| {.x = x},
                [42]
            );
        ''')

        self.assertEqual(await client.query('.x'), 8)
        await asyncio.sleep(8)
        self.assertEqual(await client.query('.x'), 42)

        await client.query(r'''
            .x = 8;
            new_timer(
                datetime().move('seconds', 2),
                |x| {.x = x},
                []
            );
        ''')

        self.assertEqual(await client.query('.x'), 8)
        await asyncio.sleep(8)
        self.assertIs(await client.query('.x'), None)

        res = await client.query(r'''
            .x = 8;
            timer = new_timer(
                datetime().move('seconds', 2),
                |x| {.x = x}
            );
            set_timer_args(timer, [42, 123]);
            timer_args(timer);
        ''')

        self.assertEqual(res, [42])

        self.assertEqual(await client.query('.x'), 8)
        await asyncio.sleep(8)
        self.assertEqual(await client.query('.x'), 42)

        res = await client.query(r'''
            .x = 8;
            timer = new_timer(
                datetime().move('seconds', 2),
                || {.x += 8}
            );
            wse(run(timer));
        ''')
        self.assertEqual(res, 16)
        await asyncio.sleep(8)
        self.assertEqual(await client.query('.x'), 24)

        await asyncio.sleep(8)
        self.assertEqual(await client.query('.r'), 2)

        res = await client.query(r'''
            timer = new_timer(
                datetime(),
                || {
                    future(|| {
                        .result = 'WoW!!';
                    });
                }
            );
        ''')

        await asyncio.sleep(8)
        self.assertEqual(await client.query('.result'), 'WoW!!')

    async def test_timer_args(self, client):
        with self.assertRaisesRegex(
                LookupError,
                r'function `timer_args` is undefined in the `@node` scope; '
                r'you might want to query the `@thingsdb` or '
                r'a `@collection` scope\?'):
            await client.query('timer_args();', scope='/n')

        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `timer_args`'):
            await client.query('nil.timer_args();')

        with self.assertRaisesRegex(
                LookupError,
                'missing timer; use this function within a timer callback or '
                'provide a timer ID as first argument'):
            await client.query('timer_args();')

        with self.assertRaisesRegex(
                LookupError,
                '`timer:4` not found'):
            await client.query('timer_args(4);')

        timer = await client.query(r'''
            new_timer(timeval().move('days', 1), || nil, [1, 2, 3]);
        ''')
        res = await client.query('timer_args(timer);', timer=timer)
        self.assertEqual(res, [])

        timer = await client.query(r'''
            new_timer(timeval().move('days', 1), |x, y, z| nil, [1, 2]);
        ''')
        res = await client.query('timer_args(timer);', timer=timer)
        self.assertEqual(res, [1, 2, None])

        with self.assertRaisesRegex(
                TypeError,
                'type `tuple` does not support index assignments'):
            await client.query('timer_args(timer)[0] = 42;', timer=timer)

        timer = await client.query(r'''
            new_timer(
                timeval(),
                |x| .x = timer_args(),
                ['MTB']
            );
        ''')
        await asyncio.sleep(8)

        res = await client.query('.x')
        self.assertEqual(res, ['MTB'])


if __name__ == '__main__':
    run_test(TestTimers())

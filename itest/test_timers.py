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

    @default_test_setup(num_nodes=2, seed=1, threshold_full_storage=10)
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
                |timer, x| {
                    .r = x;
                    set_timer_args(timer, [x+1]);
                },
                [1]
            );
        ''')

        await client.query(r'''
            .x = 8;
            new_timer(
                datetime().move('seconds', 2),
                nil,
                |timer, x| {.x = x.value},
                [{
                    value: 42
                }]
            );
        ''')

        self.assertEqual(await client.query('.x'), 8)
        await asyncio.sleep(8)
        self.assertEqual(await client.query('.x'), 42)

        await client.query(r'''
            .x = 8;
            new_timer(
                datetime().move('seconds', 2),
                |timer, x| {.x = x},
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
                |timer, x| {.x = x}
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
            await client.query('timer_args(123);', scope='/n')

        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `timer_args`'):
            await client.query('nil.timer_args();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `timer_args` takes 1 argument but 0 were given;'):
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
            new_timer(timeval().move('days', 1), |_, x, y, z| nil, [1, 2]);
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
                |timer, x| .x = timer_args(timer),
                ['MTB']
            );
        ''')
        await asyncio.sleep(8)

        res = await client.query('.x')
        self.assertEqual(res, ['MTB'])

    async def test_del_timer(self, client):
        with self.assertRaisesRegex(
                LookupError,
                r'function `del_timer` is undefined in the `@node` scope; '
                r'you might want to query the `@thingsdb` or '
                r'a `@collection` scope\?'):
            await client.query('del_timer(123);', scope='/n')

        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `del_timer`'):
            await client.query('nil.del_timer();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `del_timer` takes 1 argument but 0 were given;'):
            await client.query('del_timer();')

        with self.assertRaisesRegex(
                TypeError,
                'expecting type `str` or `int` as timer '
                'but got type `str` instead'):
            await client.query('del_timer("unknown");')

        t1 = await client.query(r'''
            timer = new_timer(datetime(), |timer| del_timer(timer));
        ''')
        t2 = await client.query(r'''
            timer = new_timer(datetime(), |timer| .done = true);
            del_timer(timer);
            timer;
        ''')

        await asyncio.sleep(8)

        with self.assertRaisesRegex(
                LookupError,
                f'`timer:{t1}` not found'):
            await client.query('timer_args(t);', t=t1)

        with self.assertRaisesRegex(
                LookupError,
                f'`timer:{t2}` not found'):
            await client.query('timer_args(t);', t=t2)

        self.assertFalse(await client.query('.has("done");'))

    async def test_has_timer(self, client):
        with self.assertRaisesRegex(
                LookupError,
                r'function `has_timer` is undefined in the `@node` scope; '
                r'you might want to query the `@thingsdb` or '
                r'a `@collection` scope\?'):
            await client.query('has_timer(123);', scope='/n')

        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `has_timer`'):
            await client.query('nil.has_timer();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `has_timer` takes 1 argument but 0 were given;'):
            await client.query('has_timer();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `has_timer` expects argument 1 to be of '
                r'type `int` but got type `str` instead;'):
            await client.query('has_timer("unknown");')

        t1 = await client.query(r'''
            timer = new_timer(datetime().move('seconds', 2), || nil);
        ''')

        self.assertTrue(await client.query('has_timer(t);', t=t1))
        await asyncio.sleep(8)
        self.assertFalse(await client.query('has_timer(t);', t=t1))

    async def test_ti_scope(self, client):
        with self.assertRaisesRegex(
                TypeError,
                r'type `thing` is not allowed as a timer argument in '
                r'the `@thingsdb` scope;'):
            timer = await client.query(r'''
                t = {};
                t.counter = 0;
                t.me = t;
                new_timer(datetime(), 30, |_, t| {
                    t.counter += 1
                }, [t]);
            ''', scope='/t')

        timer = await client.query(r'''
            new_timer(datetime().move('seconds', 2), 30, |t, i| {
                set_timer_args(t, [i+1]);
            }, [0]);
        ''', scope='/t')

        self.assertEqual(await client.query(
            'timer_args(t);',
            t=timer, scope='/t'), [0])

        await asyncio.sleep(8)

        self.assertEqual(await client.query(
            'timer_args(t);',
            t=timer, scope='/t'), [1])

        await asyncio.sleep(30)
        self.assertEqual(await client.query(
            'timer_args(t);',
            t=timer, scope='/t'), [2])


if __name__ == '__main__':
    run_test(TestTimers())

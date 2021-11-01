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


class TestTasks(TestBase):

    title = 'Test tasks'

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

    async def test_task(self, client):
        with self.assertRaisesRegex(
                LookupError,
                r'function `task` is undefined in the `@node` scope; '
                r'you might want to query the `@thingsdb` or '
                r'a `@collection` scope\?'):
            await client.query('task();', scope='/n')

        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `task`'):
            await client.query('nil.task();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `task` requires at least 1 argument '
                'but 0 were given'):
            await client.query('task();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `task` takes at most 3 arguments '
                'but 4 were given'):
            await client.query('task(datetime(), ||nil, [], nil);')

        with self.assertRaisesRegex(
                ValueError,
                r'start time out-of-range'):
            await client.query('task(datetime(-1), ||nil);')

        with self.assertRaisesRegex(
                TypeError,
                r'cannot convert type `nil` to `task`'):
            await client.query('task(nil);')

        with self.assertRaisesRegex(
                LookupError,
                r'task with id 123456 not found'):
            await client.query('task(123456);')

        with self.assertRaisesRegex(
                TypeError,
                r'function `task` expects argument 2 to be of '
                r'type `closure` but got type `nil` instead'):
            await client.query('task(datetime(), nil);')

        with self.assertRaisesRegex(
                TypeError,
                r'function `task` expects argument 3 to be of '
                r'type `list` or `tuple` but got type `nil` instead'):
            await client.query('task(datetime(), ||nil, nil);')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'got 1 task argument while the given closure takes '
                r'at most 0 arguments'):
            await client.query('task(datetime(), ||nil, [nil]);')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'got 1 task argument while the given closure takes '
                r'at most 0 arguments'):
            await client.query('task(datetime(), |task|nil, [nil]);')

        res = await client.query(r"""//ti
            .x = 8;
            task(
                datetime().move('seconds', 2),
                |task, x| {
                    .x = x.value;
                }, [{
                    value: 42
                }]
            ).id();
        """)

        self.assertEqual(await client.query('task(id).id();', id=res), res)
        self.assertEqual(await client.query('.x'), 8)
        await asyncio.sleep(5)
        self.assertEqual(
            await client.query('try(task(id));', id=res),
            f'task with id {res} not found')
        self.assertEqual(await client.query('.x'), 42)

        task_str, task_id = await client.query(r"""//ti
            t = task(
                datetime(),
                |task, x| {
                    .x = x
                }
            );
            [str(t), t.id()];
        """)

        self.assertEqual(task_str, f'task:{task_id}')
        await asyncio.sleep(3)
        self.assertIs(await client.query('.x'), None)

        res = await client.query(r"""//ti
            task(
                datetime(),
                || future(|| .result = 'WoW!!')
            );
        """)

        await asyncio.sleep(3)
        self.assertEqual(await client.query('.result'), 'WoW!!')


if __name__ == '__main__':
    run_test(TestTasks())

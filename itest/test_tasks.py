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


num_nodes = 1


class TestTasks(TestBase):

    title = 'Test tasks'

    @default_test_setup(num_nodes=num_nodes, seed=1, threshold_full_storage=10)
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
        await asyncio.sleep(num_nodes*5)
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
        await asyncio.sleep(num_nodes*3)
        self.assertIs(await client.query('.x'), None)

        res = await client.query(r"""//ti
            task(
                datetime(),
                || future(|| .result = 'WoW!!')
            );
        """)

        await asyncio.sleep(num_nodes*3)
        self.assertEqual(await client.query('.result'), 'WoW!!')

    async def test_is_task(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `is_task` takes 1 argument but 0 were given'):
            await client.query('is_task();')

        self.assertTrue(await client.query('is_task(task(datetime(), ||0));'))
        self.assertFalse(await client.query('is_task( "bla" ); '))

    async def test_task_bool(self, client):
        self.assertTrue(await client.query('bool(task(datetime(), ||0));'))
        await client.query("""//ti
            .t = task(datetime(), ||1/1);
            .f = task(datetime(), ||0/0);
        """)
        self.assertTrue(await client.query('bool(.t);'))
        self.assertTrue(await client.query('bool(.f);'))
        self.assertFalse(await client.query('is_err(.t.err());'))
        self.assertFalse(await client.query('is_err(.f.err());'))
        await asyncio.sleep(num_nodes*3)
        self.assertFalse(await client.query('bool(.t);'))
        self.assertFalse(await client.query('bool(.f);'))
        self.assertFalse(await client.query('is_err(.t.err());'))
        self.assertTrue(await client.query('is_err(.f.err());'))

    async def test_tasks(self, client):
        with self.assertRaisesRegex(
                LookupError,
                r'function `tasks` is undefined in the `@node` scope; '
                r'you might want to query the `@thingsdb` or '
                r'a `@collection` scope\?'):
            await client.query('tasks();', scope='/n')

        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `tasks`'):
            await client.query('nil.tasks();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `tasks` takes 0 arguments '
                'but 2 were given'):
            await client.query('tasks(0, 1);')

        self.assertEqual(await client.query('tasks();', scope='/t'), [])
        self.assertEqual(await client.query('tasks();', scope='//stuff'), [])

    async def test_again_in(self, client):
        await client.query("""//ti
            s = datetime();
            .t1 = task(s, |t| t.again_in());
            .t2 = task(s, |t| t.again_in('seconds', 5, nil));
            .t3 = task(s, |t| t.again_in(5, 'seconds'));
            .t4 = task(s, |t| t.again_in('s', 5));
            .t5 = task(s, |t| t.again_in('seconds', nil));
            .t6 = task(s, |t| t.again_in('seconds', 0));
        """)

        await asyncio.sleep(num_nodes*3)

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `again_in` takes 2 arguments but 0 were given;'):
            await client.query('raise(.t1.err());')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `again_in` takes 2 arguments but 3 were given;'):
            await client.query('raise(.t2.err());')

        with self.assertRaisesRegex(
                TypeError,
                r'function `again_in` expects argument 1 to be of '
                r'type `str` but got type `int` instead;'):
            await client.query('raise(.t3.err());')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid unit, expecting one of `years`, `months`, `weeks`, '
                r'`days`, `hours`, `minutes` or `seconds`'):
            await client.query('raise(.t4.err());')

        with self.assertRaisesRegex(
                TypeError,
                r'function `again_in` expects argument 2 to be of '
                r'type `int` but got type `nil` instead;'):
            await client.query('raise(.t5.err());')

        with self.assertRaisesRegex(
                ValueError,
                r'start time out-of-range;'):
            await client.query('raise(.t6.err());')

        await client.query("""//ti
            .x = 1;
            task(datetime(), |t| {
                if (.x >= 3, {
                    return();
                });
                .x += 1;
                t.again_in('seconds', 1);
            });
        """)
        await asyncio.sleep(num_nodes*5)
        self.assertEqual(await client.query('.x;'), 3)

    async def test_again_at(self, client):
        await client.query("""//ti
            s = datetime();
            .t1 = task(s, |t| t.again_at());
            .t2 = task(s, |t| t.again_at(datetime(), nil));
            .t3 = task(s, |t| t.again_at(now()));
            .t4 = task(s, |t| t.again_at(t.at()));
        """)

        await asyncio.sleep(num_nodes*3)

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `again_at` takes 1 argument but 0 were given;'):
            await client.query('raise(.t1.err());')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `again_at` takes 1 argument but 2 were given;'):
            await client.query('raise(.t2.err());')

        with self.assertRaisesRegex(
                TypeError,
                r'function `again_at` expects argument 1 to be of '
                r'type `datetime` or `timeval` but got type `float` instead;'):
            await client.query('raise(.t3.err());')

        with self.assertRaisesRegex(
                ValueError,
                r'start time out-of-range;'):
            await client.query('raise(.t4.err());')

        await client.query("""//ti
            .x = 1;
            task(datetime(), |t| {
                if (.x >= 3, {
                    return();
                });
                .x += 1;
                t.again_at(datetime().move('seconds', 1));
            });
        """)
        await asyncio.sleep(num_nodes*5)
        self.assertEqual(await client.query('.x;'), 3)

    async def test_task_with_future(self, client):
        await client.query("""//ti
            .x = 1;
            task(datetime(), |t| {
                if (.x >= 3, {
                    return();
                });

                // no change id, yet...
                assert(is_nil(change_id()));

                future(|t| {
                    .x += 1;
                    future(|t| {
                        t.again_in('seconds', 1);
                    });
                });
            });
        """)
        await asyncio.sleep(num_nodes*5)
        self.assertEqual(await client.query('.x;'), 3)

        await client.query("""//ti
            .y = 1;
            task(datetime(), |t| {
                if (.y >= 3, {
                    return();
                });

                .y += 1;

                future(|t| {
                    t.again_in('seconds', 1);
                });
            });
        """)
        await asyncio.sleep(num_nodes*5)
        self.assertEqual(await client.query('.y;'), 3)

    async def test_repr_task(self, client):
        tasks = await client.query("""//ti
            .tasks = [
                task(datetime(), |t| t.again_in('seconds', 1)),
                task(datetime(), ||nil),
                task(datetime(), ||0/0),
            ];
            .tasks.map(|t| [t.id(), str(t)]);
        """)

        for t in tasks:
            self.assertTrue(isinstance(t[0], int))
            self.assertEqual(f'task:{t[0]}', t[1])

        await asyncio.sleep(num_nodes*3)
        tasks = await client.query("""//ti
            .tasks.map(|t| str(t));
        """)

        self.assertNotEqual(f'task:nil', tasks[0])
        self.assertEqual(f'task:nil', tasks[1])
        self.assertNotEqual(f'task:nil', tasks[2])


if __name__ == '__main__':
    run_test(TestTasks())
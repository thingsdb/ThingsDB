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
from thingsdb.exceptions import IndexError
from thingsdb.exceptions import OverflowError
from thingsdb.exceptions import ZeroDivisionError
from thingsdb import scope


class TestProcedures(TestBase):

    title = 'Test nested and more complex queries'

    @default_test_setup(num_nodes=5, seed=1)
    async def run(self):

        await self.node0.init_and_run()

        client0 = await get_client(self.node0)
        client0.use('stuff')

        with self.assertRaisesRegex(
                IndexError,
                r'function `new_procedure` is undefined in the `node` scope'):
            await client0.query(r'''
                new_procedure('foo', ||nil);
            ''', target=scope.node)

        await client0.query(r'''
            new_procedure('create_user', |user| {
                new_user(user);
                token = new_token(user);
                grant('.node', user, (READ|WATCH));
                grant('stuff', user, (MODIFY|RUN));
                token;
            });
        ''', target=scope.thingsdb)

        iris_token = await client0.run(
            'create_user',
            'iris',
            target=scope.thingsdb)

        await client0.query(r'''
            .r = 'Test Raw String';
            .t = {test: 'Thing', nested: {found: true}};
            .s = set([{test: 'Thing in Set'}]);
            .f = 3.14;
            .b = true;
            .n = nil;
            .a = [[true, false], [false, true]];
            .l = [1, 2, 3];

            .upd_list = |i| .l = .l.map(|x| (x + i));

            new_procedure('upd_list', |i| wse({
                "Add a given value `i` to all values in `.l`";
                .upd_list.call(i);
                nil;
            }));

            new_procedure('missing_wse', |i| .upd_list.call(i));

            new_procedure('get_first', ||.l[0]);

            new_procedure('validate', || (
                .r == 'Test Raw String' &&
                .t.test == 'Thing' &&
                .s.find(||true).test == 'Thing in Set' &&
                .f == 3.14 &&
                .b == true &&
                .n == nil &&
                .a == [[true, false], [false, true]]
            ));

            new_procedure('default_deep', || {
                /* Return one level deep. */
                .t;
            });

            new_procedure('deep_two', || {
                "Return two levels deep.";
                .t;
                => 2
            });
        ''')

        self.assertEqual(
            await client0.query('procedure_doc("upd_list");'),
            "Add a given value `i` to all values in `.l`"
        )

        # add another node for query validation
        await self.node1.join_until_ready(client0)
        await self.node2.join_until_ready(client0)

        client1 = await get_client(self.node1, auth=iris_token)
        client1.use('stuff')

        client2 = await get_client(self.node2, auth=iris_token)
        client2.use('stuff')

        for client in (client0, client1, client2):
            self.assertTrue(await client.run('validate'))

        for client in (client0, client1, client2):
            t = await client.run('default_deep')
            self.assertIs(t['nested'].get('found'), None)
            t = await client.run('deep_two')
            self.assertIs(t['nested'].get('found'), True)

        for client in (client0, client1, client2):
            with self.assertRaisesRegex(
                    BadDataError,
                    r'stored closures with side effects must be wrapped '
                    r'using `wse\(...\)`'):
                await client.run('missing_wse', 1)

        for client in (client0, client1, client2):
            with self.assertRaisesRegex(
                    IndexError,
                    r'argument `0` for procedure `upd_list` is invalid '
                    r'\(or missing\)'):
                await client.run('upd_list')

            with self.assertRaisesRegex(
                    IndexError,
                    r'too much arguments for procedure `upd_list`'):
                await client.run('upd_list', 1, 2)

        first = 1
        for client in (client0, client1, client2):
            first += 2
            await client.run('upd_list', 2)

        await asyncio.sleep(0.2)
        for client in (client0, client1, client2):
            self.assertEqual(await client.run('get_first'), first)

        for client in (client0, client1, client2):
            first += 3
            await client.query(r'''
                wse(.upd_list.call(3));
                nil;
            ''')

        await asyncio.sleep(0.2)
        for client in (client0, client1, client2):
            self.assertEqual(await client.run('get_first'), first)

        # force a full database store
        for x in range(10):
            await client.query(f'.x = {x};')

        # add two more nodes for testing full storage sync
        await self.node3.join_until_ready(client0)
        await self.node4.join_until_ready(client0)

        client3 = await get_client(self.node3, auth=iris_token)
        client3.use('stuff')

        client4 = await get_client(self.node4, auth=iris_token)
        client4.use('stuff')

        for client in (client0, client1, client2, client3, client4):
            self.assertTrue(await client.run('validate'))
            self.assertEqual(await client.run('get_first'), first)
            self.assertEqual(await client.query('.x'), x)

        for client in (client0, client1, client2, client3, client4):
            self.assertEqual(
                await client.query('procedure_doc("upd_list");'),
                "Add a given value `i` to all values in `.l`"
            )

            procedure_info = await client.query('procedure_info("upd_list");')
            self.assertTrue(procedure_info['with_side_effects'])

            procedure_info = await client.query('procedure_info("validate");')
            self.assertFalse(procedure_info['with_side_effects'])

            self.assertEqual(
                await client.query('procedure_doc("default_deep");'),
                "/* Return one level deep. */"
            )

            self.assertEqual(
                await client.query('procedure_doc("deep_two");'),
                "Return two levels deep."
            )

        self.assertIs(await client.query(r'''
            del_procedure('missing_wse');
        '''), None)

        await asyncio.sleep(0.2)
        for client in (client0, client1, client2, client3, client4):
            self.assertTrue(await client.query(
                '(procedures_info().len() == 5);'))

        # run tests
        await self.run_tests(client0)

        # expected no garbage collection or failed events
        for client in (client0, client1, client2, client3, client4):
            counters = await client.query('counters();', target=scope.node)
            self.assertEqual(counters['garbage_collected'], 0)
            self.assertEqual(counters['events_failed'], 0)

            client.close()
            await client.wait_closed()

    async def test_run(self, client):
        await client.query(r'''
            new_procedure('test', |x| (x * 10));
            new_procedure('test_wse', |x| .x = x);
        ''')

        with self.assertRaisesRegex(
                IndexError,
                'type `closure` has no function `run`'):
            await client.query('(||nil).run();')

        with self.assertRaisesRegex(
                BadDataError,
                'function `run` requires at least 1 argument '
                'but 0 were given'):
            await client.query('run();')

        with self.assertRaisesRegex(
                BadDataError,
                r'function `run` expects argument 1 to be of type `raw` '
                r'but got type `nil` instead'):
            await client.query('run(nil);')

        with self.assertRaisesRegex(
                IndexError,
                r'procedure `xxx` not found'):
            await client.query('run("xxx");')

        with self.assertRaisesRegex(
                BaseException,
                r'procedure must be a valid name'):
            await client.query('run("");')

        with self.assertRaisesRegex(
                BadDataError,
                r'stored closures with side effects must be wrapped '
                r'using `wse\(...\)`'):
            await client.query(r'''
                run('test_wse', 42)
            ''')

        with self.assertRaisesRegex(
                BadDataError,
                'this closure takes 1 argument but 0 were given'):
            await client.query(r'''
                run('test');
            ''')

        self.assertEqual(await client.query('run("test", 6);'), 60)
        self.assertEqual(await client.query('wse(run("test_wse", 42));'), 42)


if __name__ == '__main__':
    run_test(TestProcedures())

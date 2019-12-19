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


class TestProcedures(TestBase):

    title = 'Test procedures'

    @default_test_setup(num_nodes=5, seed=1)
    async def run(self):

        await self.node0.init_and_run()

        client0 = await get_client(self.node0)
        client0.set_default_scope('@:stuff')

        with self.assertRaisesRegex(
                LookupError,
                r'function `new_procedure` is undefined in the `@node` scope'):
            await client0.query(r'''
                new_procedure('foo', ||nil);
            ''', scope='@node')

        await client0.query(r'''
            new_procedure('create_user', |user| {
                "Create a user with a token and basic privileges.";
                new_user(user);
                token = new_token(user);
                grant('@node', user, (READ|WATCH));
                grant('@:stuff', user, (MODIFY|RUN));
                token;
            });
        ''', scope='@thingsdb')

        iris_token = await client0.run(
            'create_user',
            'iris',
            scope='@thingsdb')

        await client0.query(r'''
            // First set properties which we can later verify
            .r = 'Test Raw String';
            .t = {test: 'Thing', nested: {found: true}};
            .s = set({test: 'Thing in Set'},);
            .f = 3.14;
            .b = true;
            .n = nil;
            .a = [[true, false], [false, true]];
            .l = [1, 2, 3];

            // Create a closure
            .upd_list = |i| .l = .l.map(|x| (x + i));

            // Create a procedure which calls a closure
            new_procedure('upd_list', |i| wse({
                "Add a given value `i` to all values in `.l`";
                .upd_list.call(i);
                nil;  // Return nil
            }));

            /*********************************
             * Create some extra procedures. *
             *********************************/
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
                "Return one level deep.";
                .t;  // Return `t`
            });

            new_procedure('deep_two', || {
                "Return two levels deep.";
                return(thing(.t.id()), 2);
            });
        ''')

        self.assertEqual(
            await client0.query('procedure_doc("upd_list");'),
            "Add a given value `i` to all values in `.l`"
        )

        with self.assertRaisesRegex(
                ValueError,
                r'property name must follow the naming rules'
                r'; see.*'
                r'\(argument 0 for procedure `upd_list`\)'):
            await client0.run('upd_list', {"0InvalidKey": 4})

        with self.assertRaisesRegex(
                LookupError,
                r'thing `#42` not found; if you want to create a new thing '
                r'then remove the id \(`#`\) and try again '
                r'\(argument 0 for procedure `upd_list`\)'):
            await client0.run('upd_list', {"#": 42})

        with self.assertRaisesRegex(
                TypeError,
                r'sets can only contain things '
                r'\(argument 0 for procedure `upd_list`\)'):
            await client0.run('upd_list', {"$": [1, 2, 3]})

        # add another node for query validation
        await self.node1.join_until_ready(client0)
        await self.node2.join_until_ready(client0)

        client1 = await get_client(self.node1, auth=iris_token)
        client1.set_default_scope('//stuff')

        client2 = await get_client(self.node2, auth=iris_token)
        client2.set_default_scope('//stuff')

        for client in (client0, client1, client2):
            self.assertTrue(await client.run('validate'))

        for client in (client0, client1, client2):
            t = await client.run('default_deep')
            self.assertIs(t['nested'].get('found'), None)
            t = await client.run('deep_two')
            self.assertIs(t['nested'].get('found'), True)

        for client in (client0, client1, client2):
            with self.assertRaisesRegex(
                    OperationError,
                    r'stored closures with side effects must be wrapped '
                    r'using `wse\(...\)`'):
                await client.run('missing_wse', 1)

        for client in (client0, client1, client2):
            with self.assertRaisesRegex(
                    NumArgumentsError,
                    r'procedure `upd_list` takes 1 argument but 0 were given'):
                await client.run('upd_list')

            with self.assertRaisesRegex(
                    NumArgumentsError,
                    r'procedure `upd_list` takes 1 argument but 2 were given'):
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

        # force a full database store ✅
        for x in range(10):
            await client.query(f'.x = {x};')

        # add two more nodes for testing full storage sync
        await self.node3.join_until_ready(client0)
        await self.node4.join_until_ready(client0)

        client3 = await get_client(self.node3, auth=iris_token)
        client3.set_default_scope('//stuff')

        client4 = await get_client(self.node4, auth=iris_token)
        client4.set_default_scope('//stuff')

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
                "Return one level deep."
            )

            self.assertEqual(
                await client.query('procedure_doc("validate");'), "")

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
            counters = await client.query('counters();', scope='@node')
            self.assertEqual(counters['garbage_collected'], 0)
            self.assertEqual(counters['events_failed'], 0)

            client.close()
            await client.wait_closed()

    async def test_del_procedure(self, client):
        await client.query(r'''
            new_procedure('test', |x| (x * 10));
        ''')

        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `del_procedure`'):
            await client.query('nil.del_procedure();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `del_procedure` takes 1 argument '
                'but 0 were given'):
            await client.query('del_procedure();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `del_procedure` expects argument 1 to be of '
                r'type `str` but got type `nil` instead'):
            await client.query('del_procedure(nil);')

        with self.assertRaisesRegex(
                LookupError,
                r'procedure `xxx` not found'):
            await client.query('del_procedure("xxx");')

        with self.assertRaisesRegex(
                ValueError,
                r'procedure name must follow the naming rules'):
            await client.query('del_procedure("");')

        self.assertIs(await client.query('del_procedure("test");'), None)

    async def test_has_procedure(self, client):
        await client.query(r'''
            new_procedure('test', |x| (x * 10));
        ''')

        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `has_procedure`'):
            await client.query('nil.has_procedure();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `has_procedure` takes 1 argument but 0 were given'):
            await client.query('has_procedure();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `has_procedure` expects argument 1 to be of '
                r'type `str` but got type `int` instead'):
            await client.query('has_procedure(0);')

        self.assertTrue(await client.query(r'''has_procedure('test');'''))
        self.assertFalse(await client.query(r'''has_procedure('test0');'''))

    async def test_new_procedure(self, client):
        await client.query(r'''
            new_procedure('test', |x| (x * 10));
        ''')

        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `new_procedure`'):
            await client.query('nil.new_procedure();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `new_procedure` takes 2 arguments '
                'but 1 was given'):
            await client.query('new_procedure("create_user");')

        with self.assertRaisesRegex(
                TypeError,
                r'function `new_procedure` expects argument 1 to be of '
                r'type `str` but got type `nil` instead'):
            await client.query('new_procedure(nil, ||nil);')

        with self.assertRaisesRegex(
                TypeError,
                r'function `new_procedure` expects argument 2 to be '
                r'a `closure` but got type `nil` instead'):
            await client.query('new_procedure("create_user", nil);')

        with self.assertRaisesRegex(
                BaseException,
                r'procedure name must follow the naming rules'):
            await client.query('new_procedure("", ||nil);')

        with self.assertRaisesRegex(
                LookupError,
                r'procedure `create_user` already exists'):
            await client.query(
                'new_procedure("create_user", ||nil);',
                scope='@thingsdb')

        self.assertEqual(await client.query(r'''
            new_procedure("create_user", |user| {
                .has('users') || {
                    .users = [];
                };
                new_user = {
                    name: user
                };
                .users.push(new_user);
                new_user;
            });
        '''), 'create_user')

        iris = await client.run('create_user', 'Iris')
        self.assertEqual(iris['name'], 'Iris')

    async def test_procedure_doc(self, client):
        await client.query(r'''
            new_procedure('square', |x| {
                "No side effects.";
                (x * x);
            });
            new_procedure('set_a', |a| {
                "With side effects.";
                .a = a;
            });
        ''')

        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `procedure_doc`'):
            await client.query('nil.procedure_doc();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `procedure_doc` takes 1 argument but 0 were given'):
            await client.query('procedure_doc();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `procedure_doc` expects argument 1 to be of '
                r'type `str` but got type `float` instead'):
            await client.query('procedure_doc(3.14);')

        with self.assertRaisesRegex(
                LookupError,
                r'procedure `xxx` not found'):
            await client.query('procedure_doc("xxx");')

        with self.assertRaisesRegex(
                BaseException,
                r'procedure name must follow the naming rules'):
            await client.query('procedure_doc("!test");')

        self.assertEqual(
            await client.query('procedure_doc("square");'),
            'No side effects.')

        self.assertEqual(
            await client.query('procedure_doc("set_a");'),
            'With side effects.')

    async def test_procedure_info(self, client):

        await client.query(r'''
            new_procedure('square', |x| {
                "No side effects.";
                (x * x);
            });
            new_procedure('set_a', |a| {
                "With side effects.";
                .a = a;
            });
        ''')

        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `procedure_info`'):
            await client.query('nil.procedure_info();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `procedure_info` takes 1 argument but 0 were given'):
            await client.query('procedure_info();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `procedure_info` expects argument 1 to be of '
                r'type `str` but got type `float` instead'):
            await client.query('procedure_info(3.14);')

        with self.assertRaisesRegex(
                LookupError,
                r'procedure `xxx` not found'):
            await client.query('procedure_info("xxx");')

        with self.assertRaisesRegex(
                BaseException,
                r'procedure name must follow the naming rules'):
            await client.query('procedure_info("0123");')

        procedure_info = await client.query('procedure_info("square");')
        self.assertEqual(len(procedure_info), 5)
        self.assertEqual(procedure_info['with_side_effects'], False)
        self.assertEqual(procedure_info['arguments'], ['x'])
        self.assertEqual(procedure_info['name'], 'square')
        self.assertEqual(procedure_info['doc'], 'No side effects.')
        self.assertTrue(isinstance(procedure_info['definition'], str))

        procedure_info = await client.query('procedure_info("set_a");')
        self.assertEqual(len(procedure_info), 5)
        self.assertEqual(procedure_info['with_side_effects'], True)
        self.assertEqual(procedure_info['arguments'], ['a'])
        self.assertEqual(procedure_info['name'], 'set_a')
        self.assertEqual(procedure_info['doc'], 'With side effects.')
        self.assertTrue(isinstance(procedure_info['definition'], str))

    async def test_procedures_info(self, client):
        await client.query(r'''
            new_procedure('square', |x| {
                "No side effects.";
                (x * x);
            });
            new_procedure('set_a', |a| {
                "With side effects.";
                .a = a;
            });
        ''')

        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `procedures_info`'):
            await client.query('nil.procedures_info();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `procedures_info` takes 0 arguments '
                'but 1 was given'):
            await client.query('procedures_info("set_a");')

        procedures_info = await client.query('procedures_info();')
        self.assertEqual(len(procedures_info), 2)
        for info in procedures_info:
            self.assertEqual(len(info), 5)
            self.assertEqual(len(info['arguments']), 1)
            self.assertTrue(isinstance(info['with_side_effects'], bool))
            self.assertTrue(isinstance(info['name'], str))
            self.assertTrue(isinstance(info['doc'], str))
            self.assertTrue(isinstance(info['definition'], str))

    async def test_run(self, client):
        await client.query(r'''
            new_procedure('test', |x| (x * 10));
            new_procedure('test_wse', |x| .x = x);
        ''')

        with self.assertRaisesRegex(
                LookupError,
                'type `closure` has no function `run`'):
            await client.query('(||nil).run();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `run` requires at least 1 argument '
                'but 0 were given'):
            await client.query('run();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `run` expects argument 1 to be of type `str` '
                r'but got type `nil` instead'):
            await client.query('run(nil);')

        with self.assertRaisesRegex(
                LookupError,
                r'procedure `xxx` not found'):
            await client.query('run("xxx");')

        with self.assertRaisesRegex(
                BaseException,
                r'procedure name must follow the naming rules'):
            await client.query('run("");')

        with self.assertRaisesRegex(
                OperationError,
                r'stored closures with side effects must be wrapped '
                r'using `wse\(...\)`'):
            await client.query(r'''
                run('test_wse', 42)
            ''')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'this closure takes 1 argument but 0 were given'):
            await client.query(r'''
                run('test');
            ''')

        self.assertEqual(await client.query('run("test", 6);'), 60)
        self.assertEqual(await client.query('wse(run("test_wse", 42));'), 42)

    async def test_thing_argument(self, client):
        await client.query(r'''
            new_procedure('test_save_thing', |t| .t = t);
            new_procedure('test_another_thing', |t| .other = t);
            new_procedure('as_named_args', |options| .name = options.name);
        ''')

        await client.run('test_save_thing', {'name': 'Iris', 'age': 6})
        await client.run('as_named_args', {'name': 'Cato'})

        iris = await client.query('.t')

        self.assertEqual(iris['name'], 'Iris')
        self.assertEqual(iris['age'], 6)
        self.assertEqual(await client.query('.name'), 'Cato')

        await client.run('test_another_thing', iris)

        self.assertTrue(await client.query('(.other == .t)'))


if __name__ == '__main__':
    run_test(TestProcedures())

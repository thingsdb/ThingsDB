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


class TestAdvanced(TestBase):

    title = 'Test advanced, single node'

    @default_test_setup(num_nodes=1, seed=1, threshold_full_storage=10)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_query_gc(self, client):
        self.assertEqual(await client.query(r'''
            x = {};
            x.x = x;
            x = 5;
        '''), 5)

        self.assertEqual(await client.query(r'''
            x = {};
            x.y = {};
            x.y.y = x.y;
            x.set('y', 5);
        '''), 5)

        self.assertEqual(await client.query(r'''
            x = [{}];
            x[0].x = x[0];
            x.pop();
        '''), {"x": {}})

        self.assertEqual(await client.query(r'''
            {
                x = [{}];
                x[0].x = x[0];
                x.pop();
            };
        '''), {"x": {}})

        self.assertEqual(await client.query(r'''
            {
                x = [{}];
                x[0].x = x[0];
                x.pop();
            };
            5;
        '''), 5)

        self.assertEqual(await client.query(r'''
            {
                {
                    x = [{}];
                    x[0].x = x[0];
                    x.pop();
                };
                5;
            }
        '''), 5)

        self.assertIs(await client.query(r'''
            !!{
                x = [{}];
                bool(x[0].x = x[0]);
                x.pop()
            };
        '''), True)

        self.assertIs(await client.query(r'''
            {
                x = [{}];
                bool(x[0].x = x[0]);
                x.pop()
            } && true;
        '''), True)

        self.assertIs(await client.query(r'''
            {
                x = [{}];
                bool(x[0].x = x[0]);
                x.pop()
            }.id();
        '''), None)

        self.assertEqual(await client.query(r'''
            x = {
                y: {}
            };
            x.y.x = x;
            x.y.z = {};
            x.y.z.z = x.y.z;
            x.y.z.arr = [x, x.y, x.y.z];
            5;
        '''), 5)

        self.assertEqual(await client.query(r'''
            x = {};
            x.  y = {};
            x.y.y = x.y;
            {x.del('y')}.id();
            5;
        '''), 5)

    async def test_mod_type_mod_advanced2(self, client):
        with self.assertRaisesRegex(
                OperationError,
                r'field `chat` on type `Room` is modified but at least one '
                r'new instance was made with an inappropriate value which in '
                r'response is changed to default by ThingsDB; mismatch in '
                r'type `Room`; type `int` is invalid for property `chat` with '
                r'definition `Room\?`'):
            await client.query(r'''
                set_type('Chat', {
                    messages: '[str]'
                });

                set_type('Room', {
                    name: 'str',
                    chat: 'str'
                });

                .room_a = Room{name: 'room A'};
                .room_b = Room{name: 'room B'};

                mod_type('Room', 'mod', 'chat', 'Room?', |room| {
                    // Here, we can create "Room" since chat allows "any"
                    // value at this point. However, new rooms must be modified
                    // accordinly afterwards
                    Room{name: `sub{room.name}`, chat: 123};
                });
            ''')

        msg = await client.query('.room_a.chat.name;')
        self.assertEqual(msg, 'subroom A')

        msg = await client.query('.room_b.chat.name;')
        self.assertEqual(msg, 'subroom B')

        self.assertIs(await client.query('.room_a.chat.chat;'), None)
        self.assertIs(await client.query('.room_b.chat.chat;'), None)

    async def test_type_count(self, client):
        res = await client.query(r'''
            new_type("X");
            set_type("X", {other: 'X?'});

            x = X{};
            a = [X{}];
            s = set(X{});
            y = X{};
            y.other = X{};
            t = {
                x: X{},
                a: [X{}]
            };

            .x = X{};
            .a = [X{}];
            .s = set(X{});
            .y = X{};
            .y.other = X{};

            type_count('X');
        ''')
        self.assertEqual(res, 12)

        res = await client.query(r'''
            new_type("Y");
            set_type("Y", {other: 'Y?'});

            y = Y{};
            y.other = y;

            r = {};
            r.r = r;
            x = {
                y: y,
                r: r
            };

            type_count('Y');
        ''')
        self.assertEqual(res, 1)

    async def test_mod_to_any(self, client):
        res = await client.query('''
            set_type('X', {
                arr: '[int]'
            });
            .x = X{};

            mod_type('X', 'mod', 'arr', 'any');

            .x.arr.push('Hello!');
            .x.arr.len();
        ''')
        self.assertEqual(res, 1)

    async def test_mod_del_in_use(self, client):
        with self.assertRaisesRegex(
                OperationError,
                r'cannot change type `X` while one of the '
                r'instances is being used'):
            res = await client.query('''
                set_type('X', {
                    a: 'int',
                    b: 'int',
                    c: 'int'
                });
                .x = X{};
                i = 0;
                .x.map(|k, v| {
                    if(i == 0, mod_type('X', 'del', 'c'));
                    i += 1;
                });
            ''')

    async def test_new(self, client):
        res = await client.query('''
            set_type('Count', {
                arr: '[int]'
            });

            x = {arr: [1, 2, 3]};

            c = new('Count', x);
            c.arr.push(4);
            x.arr;
        ''')
        self.assertEqual(res, [1, 2, 3])

    async def test_reuse_var(self, client):
        res = await client.query('''
            x = true;
            count = refs(true);
            x = false;
            assert (refs(true) < count);
        ''')
        self.assertIs(res, None)

    async def test_array_arg(self, client):
        res = await client.query('''
            new_procedure('add', |arr, v| arr.push(v));
            .arr = range(3);
            run('add', .arr, 3);
            .arr;
        ''')
        self.assertEqual(res, list(range(3)))

        res = await client.query('''
            .arr = range(5);
            (|arr, v| arr.push(v)).call(.arr, 5);
            .arr;
        ''')
        self.assertEqual(res, list(range(5)))

    async def test_type_mod(self, client):
        await client.query('''
            set_type("A", {b: 'str'});
            set_type("B", {a: 'A'});
            set_type("X", {arr: '[str]'});
            .x = X{arr: []};
        ''')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid declaration for `b` on type `A`; '
                r'missing `\?` after declaration `B`; '
                r'circular dependencies must be nillable at least '
                r'at one point in the chain'):
            await client.query('''
                mod_type('A', 'mod', 'b', 'B');
            ''')

        await client.query('''
            mod_type('A', 'mod', 'b', 'B?', ||nil);
        ''')

        with self.assertRaisesRegex(
                OperationError,
                r'type `B` is used by at least one other type'):
            await client.query('''
                del_type('B');
            ''')

        self.assertEqual(
            await client.query('''
                a = X{arr:[]};
                mod_type('X', 'mod', 'arr', '[str?]');
                a.arr.push(nil);
                a.arr;
            '''),
            [None])

        await client.query('''
            .x.arr.push(nil);
        ''')

    async def test_events(self, client):
        await self.assertEvent(client, r'''
            .arr = range(3);
        ''')
        await self.assertEvent(client, r'''
            thing(.id()).arr.push(3);
        ''')
        await self.assertEvent(client, r'''
            .get('arr').push(4);
        ''')
        await self.assertEvent(client, r'''
            thing(.id()).set('a', []);
        ''')

    async def test_no_events(self, client):
        await self.assertNoEvent(client, r'''
            arr = range(3);
        ''')
        await self.assertNoEvent(client, r'''
            range(3).push(4);
        ''')

    async def test_make(self, client):
        await client.query('''
            new_procedure('create_host', |
                    env_id, name, address, probes,
                    labels, description
                | {
                env = thing(env_id);
                host = {
                    parent: env,
                    name: name,
                    address: address,
                    probes: probes,
                    labels: set(labels.map(|l| thing(l))),
                    description: description,
                };
                env.hosts.add(host);
                labels.map(|l| thing(l)._hosts.add(host));
                host.id();
            });
            new_procedure('add_container', |env_id, name| {
                env = thing(env_id);
                container = {
                    parent: env,
                    name: name,
                    children: set(),
                    users: set(),
                    hosts: set(),
                    labels: set(),
                    conditions: set(),
                };
                env.children.add(container);
                container.id();
            });
            .root = {
                name: 'a',
                parent: nil,
                children: set(),
                users: set(),
                hosts: set(),
                labels: set(),
                conditions: set()
            };
        ''')
        root = await client.query('.root.id()')
        self.assertGreater(root, 0)

        a = await client.run('add_container', root, 'a')
        self.assertGreater(a, root)

        b = await client.run('add_container', a, 'b')
        self.assertGreater(b, a)

        host_a = await client.run(
            'create_host',
            root, 'name', '127.0.0.1', [], [], '')
        self.assertGreater(host_a, b)

        host_b = await client.run(
            'create_host',
            root, 'name', '127.0.0.1', [], [], '')
        self.assertGreater(host_b, host_a)


if __name__ == '__main__':
    run_test(TestAdvanced())

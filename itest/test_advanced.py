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
                r'invalid declaration for `b` on type `A`'):
            await client.query('''
                mod_type('A', 'mod', 'b', 'B');
            ''')

        await client.query('''
            mod_type('A', 'mod', 'b', 'B?');
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

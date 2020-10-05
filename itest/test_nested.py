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


class TestNested(TestBase):

    title = 'Test nested and more complex queries'

    @default_test_setup(num_nodes=3, seed=1, threshold_full_storage=500)
    async def run(self):

        await self.node0.init_and_run()

        client0 = await get_client(self.node0)
        client0.set_default_scope('//stuff')

        # add another node for query validation
        await self.node1.join_until_ready(client0)
        await self.node2.join_until_ready(client0)

        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')

        client2 = await get_client(self.node2)
        client2.set_default_scope('//stuff')

        await self.run_tests(client0, client1, client2)

        for i, client in enumerate((client0, client1, client2)):
            counters = await client.query('counters();', scope='@node')
            self.assertEqual(counters['events_failed'], 0)

            client.close()
            await client.wait_closed()

    async def test_push_loop(self, client0, client1, client2):
        await client0.query(r'''
            .arr = [1, 2];
            .map(|k, v| {
                v.push(3, 4);  // changes the copy, not the original
            });
        ''')
        await asyncio.sleep(0.1)
        for client in (client0, client1, client2):
            self.assertEqual(await client.query('.arr'), [1, 2])

    async def test_set_and_push(self, client0, client1, client2):
        await client0.query(r'''
            .set("arr", []).push(1, 2, 3, 4);
        ''')
        await asyncio.sleep(0.1)
        for client in (client0, client1, client2):
            self.assertEqual(await client.query('.arr'), [1, 2, 3, 4])

    async def test_get_and_push(self, client0, client1, client2):
        await client0.query(r'''
            .arr = [1, 2];
            .get('arr').push(3, 4);
        ''')
        await asyncio.sleep(0.1)
        for client in (client0, client1, client2):
            self.assertEqual(await client.query('.arr'), [1, 2, 3, 4])

    async def test_var_assign(self, client0, client1, client2):
        self.assertEqual(await client0.query(r'''
            .a = {};
            tmp = .a;
            tmp.test = "Test";
            .a.test;
        '''), 'Test')

        await asyncio.sleep(0.1)
        for client in (client0, client1, client2):
            self.assertEqual(await client.query('.a.test'), 'Test')

    async def test_var_assign_del(self, client0, client1, client2):
        self.assertEqual(await client0.query(r'''
            .a = {};
            tmp = .a;
            .del('a');
            tmp.x = "Still a reference";
            .b = "Test";
        '''), 'Test')

        await asyncio.sleep(0.1)
        for client in (client0, client1, client2):
            self.assertEqual(await client.query('.b'), 'Test')

    async def test_tmp_push(self, client0, client1, client2):
        # Copy is made so the `push` should not create an event
        self.assertEqual(await client0.query(r'''
            .arr = [1, 2, 3];
            arr = .arr;
            arr.push(4);
            .arr;
        '''), [1, 2, 3])

        await asyncio.sleep(0.1)
        for client in (client0, client1, client2):
            self.assertEqual(await client.query('.arr'), [1, 2, 3])

    async def test_assign_pop(self, client0, client1, client2):
        await client0.query(r'''
            .arr = [{
                name: 'Iris'
            }];
            .arr[0].age = {
                .arr.pop();
                6;
            };
        ''')
        await asyncio.sleep(0.1)
        for client in (client0, client1, client2):
            self.assertEqual(await client.query('.arr'), [])

    async def test_ret_val(self, client0, client1, client2):
        await client0.query(r'''
            .arr = [];
            .g = ||.arr;
        ''')

        arr = await client0.query(r'''.g().push(123); .arr;''')

        await asyncio.sleep(0.1)

        for client in (client0, client1, client2):
            # This should be [123] or at least [] at all nodes
            self.assertEqual(await client.query('.arr'), [123])

    async def test_assign_del(self, client0, client1, client2):
        await client0.query(r'''
            .x = {
                y: {
                    z: {}
                }
            };

            x = .x;
            y = .x.y;
            z = .x.y.z;

            z.test = {
                x.del('y');
                'Test';
            };

            x.test = 'Test';
        ''')

        await asyncio.sleep(0.1)
        for client in (client0, client1, client2):
            x = await client.query('.x')
            self.assertEqual(x['test'], 'Test')

    async def test_nested_pop(self, client0, client1, client2):
        await client0.query(r'''
            .arr = [[{
                name: 'Iris'
            }]];
            .arr[0][0].age = {
                .arr.pop();
                6;
            };
        ''')
        await asyncio.sleep(0.1)
        for client in (client0, client1, client2):
            self.assertEqual(await client.query('.arr'), [])

    async def test_list_lock_assign(self, client0, client1, client2):
        with self.assertRaisesRegex(
                OperationError,
                r'cannot change or remove property `arr` on `#\d+` while '
                r'the `list` is being used'):
            await client0.query(r'''
                .arr = ['a', 'b'];
                .arr.push({
                    .arr = [1, 2, 3];
                    'c';
                })
            ''')

    async def test_list_lock_del(self, client0, client1, client2):
        with self.assertRaisesRegex(
                OperationError,
                r'cannot change or remove property `arr` on `#\d+` while '
                r'the `list` is being used'):
            await client0.query(r'''
                .arr = ['a', 'b'];
                .arr.push({
                    .del('arr');
                    'c';
                })
            ''')

    async def test_assign_after_del(self, client0, client1, client2):
        await client0.query(r'''
            .a = {name: 'Iris'};
            tmp = .a;
            .del('a');
            .b = tmp;
        ''')

        await asyncio.sleep(0.1)
        for client in (client0, client1, client2):
            iris = await client.query('.b')
            self.assertEqual(iris['name'], 'Iris')

    async def test_assign_after_del_split(self, client0, client1, client2):
        await client0.query(r'''
            .a = {name: 'Iris'};
            .store = {};
        ''')

        await client0.query(r'''
            tmp = .a;
            .del('a');
            .store.a = tmp;
        ''')

        await asyncio.sleep(1.0)
        for client in (client0, client1, client2):
            iris = await client.query('.store.a')
            self.assertEqual(iris['name'], 'Iris')

    async def test_complex_assign_list(self, client0, client1, client2):
        with self.assertRaisesRegex(
                OperationError,
                r'cannot change or remove property `a` on `#\d+` while '
                r'the `list` is being used'):
            await client0.query(r'''
                .store = {};
                .store.a = [1, 2, 3];
                .store.a.push({
                    .store.a = 4;
                });
            ''')
        with self.assertRaisesRegex(
                OperationError,
                r'cannot change or remove property `a` on `#\d+` while '
                r'the `list` is being used'):
            await client0.query(r'''
                .store = {};
                store = .store;
                store.a = [1, 2, 3];
                store.a.push({
                    .store.del('a');
                });
            ''')

    async def test_complex_assign_set(self, client0, client1, client2):
        with self.assertRaisesRegex(
                OperationError,
                r'cannot change or remove property `a` on `#\d+` while '
                r'the `set` is being used'):
            await client0.query(r'''
                .store = {};
                .store.a = set([{}, {}, {}]);
                .store.a.add({
                    .store.a = 4;
                });
            ''')
        with self.assertRaisesRegex(
                OperationError,
                r'cannot change or remove property `a` on `#\d+` while '
                r'the `set` is being used'):
            await client0.query(r'''
                .store = {};
                store = .store;
                store.a = set([{}, {}, {}]);
                store.a.add({
                    .store.del('a');
                });
            ''')

    async def test_nested_closure_query(self, client0, client1, client2):
        usera, userb = await client0.query(r'''
            .channel = {};
            .workspace = {channels: [.channel]};
            .usera = {
                memberships: [{
                    workspace: .workspace,
                    channels: [.channel]
                }]
            };
            .userb = {
                memberships: [{
                    workspace: .workspace,
                    channels: []
                }]
            };
            .users = [.usera, .userb];
        ''')

        self.assertEqual(await client0.query(r'''
            .users.filter(
                |user| !is_nil(
                    user.memberships.find(
                        |membership| membership.workspace == .workspace
                    ).channels.index_of(.channel)
                )
            );
        '''), [usera])

        self.assertEqual(await client0.query(r'''
            .users.filter(
                |user| is_nil(
                    user.memberships.find(
                        |membership| membership.workspace == .workspace
                    ).channels.index_of(
                        .channel
                    )
                )
            );
        '''), [userb])

    async def test_set_assign(self, client0, client1, client2):
        await client0.query(r'''
            x = {n: 0}; y = {n: 1}; z = {n: 2};
            .a = set(x, y);
            .b = set(x, z);
            .a ^= .b;
        ''')

        await asyncio.sleep(1.0)

        for client in (client0, client1, client2):
            res = await client.query(r'.a.map(|x| x.n);')
            self.assertEqual(set(res), {1, 2})

    async def test_ids(self, client0, client1, client2):
        nones, ids = await client0.query(r'''
            a = {b: {c: {}}};
            a.b.c.d = {};
            .x = [a.id(), a.b.id(), a.b.c.id(), a.b.c.d.id()];
            .a = a;
            .y = [a.id(), a.b.id(), a.b.c.id(), a.b.c.d.id()];
            [.x, .y];
        ''')

        self.assertEqual(nones, [None, None, None, None])
        self.assertGreater(ids[0], 1)
        self.assertGreater(ids[1], ids[0])
        self.assertGreater(ids[2], ids[1])
        self.assertGreater(ids[3], ids[2])


if __name__ == '__main__':
    run_test(TestNested())

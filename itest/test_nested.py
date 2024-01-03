#!/usr/bin/env python
import asyncio
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client
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
            self.assertEqual(counters['changes_failed'], 0)

            client.close()
            await client.wait_closed()

    async def test_push_loop(self, client0, client1, client2):
        await client0.query(r'''
            .arr = [1, 2];
            .map(|k, v| {
                v.push(3, 4);  // this is a reference
            });
        ''')
        await asyncio.sleep(0.1)
        for client in (client0, client1, client2):
            self.assertEqual(await client.query('.arr'), [1, 2, 3, 4])

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
        '''), [1, 2, 3, 4])

        await asyncio.sleep(0.1)
        for client in (client0, client1, client2):
            self.assertEqual(await client.query('.arr'), [1, 2, 3, 4])

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
        await client0.query(r"""//ti
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
        """)

        await asyncio.sleep(0.1)
        for client in (client0, client1, client2):
            x = await client.query('.x')
            self.assertEqual(x['test'], 'Test')

    async def test_nested_pop(self, client0, client1, client2):
        await client0.query(r"""//ti
            .arr = [[{
                name: 'Iris'
            }]];
            .arr[0][0].age = {
                .arr.pop();
                6;
            };
        """)
        await asyncio.sleep(0.1)
        for client in (client0, client1, client2):
            self.assertEqual(await client.query('.arr'), [])

    async def test_list_lock_assign(self, client0, client1, client2):
        with self.assertRaisesRegex(
                OperationError,
                r'cannot change or remove property `arr` on `#\d+` while '
                r'the `list` is in use'):
            await client0.query(r"""//ti
                .arr = ['a', 'b'];
                .arr.push({
                    .arr = [1, 2, 3];
                    'c';
                })
            """)

    async def test_list_lock_del(self, client0, client1, client2):
        with self.assertRaisesRegex(
                OperationError,
                r'cannot change or remove property `arr` on `#\d+` while '
                r'the `list` is in use'):
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
                r'the `list` is in use'):
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
                r'the `list` is in use'):
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
                r'the `set` is in use'):
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
                r'the `set` is in use'):
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

    async def test_set_assign_and_rm_props(self, client0, client1, client2):
        await client0.query(r'''
            x = {n: 0}; y = {n: 1}; z = {n: 2};
            .a = set(x, y);
            .b = set(x, z);
            .a ^= .b;
            .t1 = {
                a: 0,
                b: 1,
            };
            .t2 = {
                a: 0,
                b: 1,
            };
        ''')

        # make sure both remove() and clear() always create a change
        await client0.query(r'''
            t1 = .t1;
            t1.remove(|| true);
        ''')
        await client0.query(r'''
            t2 = .t2;
            t2.clear();
        ''')

        await asyncio.sleep(1.0)

        for client in (client0, client1, client2):
            res = await client.query(r'.a.map(|x| x.n);')
            self.assertEqual(set(res), {1, 2})

            res = await client.query(r'.t1.len() + .t2.len();')
            self.assertEqual(res, 0)

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

    async def test_set_and_list_on_var(self, client0, client1, client2):
        await client0.query(r"""//ti
            .myset = set();
            .myarr = list();
        """)

        await client0.query(r"""//ti
            a = .myarr;
            wse(a.push({name: 'a'}));
        """)

        await client0.query(r"""//ti
            a = .myarr;
            wse(a.extend([{name: 'b'}, {name: 'c'}]));
        """)

        await client0.query(r"""//ti
            a = .myarr;
            wse(a.extend_unique([{name: 'd'}, {name: 'e'}]));
        """)

        await client0.query(r"""//ti
            s = .myset;
            wse(s.add(.myarr[0], .myarr[1]));
        """)

        await client0.query(r"""//ti
            s = .myset;
            wse(s ^= set(.myarr));
        """)

        res = await client0.query("""//ti
            .a1 = [1, 2, 3];
            .a2 = .a1;
            .a3 = .a2;
            a1 = .a1;
            a2 = .a2;
            a3 = .a3;
            .a1 = [2, 3, 4];
            .assign({a2: [3, 4, 5]})
            .set('a3', [4, 5, 6])

            a1.push(7);
            a2.push(8);
            a3.push(9);
            [.a1, .a2, .a3];
        """)
        self.assertEqual(res, [
            [2, 3, 4],
            [3, 4, 5],
            [4, 5, 6],
        ])

        ids = await client0.query('.myarr.map_id();')
        self.assertEqual(len(ids), 5)

        await asyncio.sleep(1.0)
        for client in (client0, client1, client2):
            size, myset = await client.query('[.myarr.len(), .myset];')
            self.assertEqual(size, 5)
            self.assertEqual(len(myset), 3)
            self.assertEqual(sorted(myset, key=lambda x: x['name']), [
                {'#': ids[2], 'name': 'c'},
                {'#': ids[3], 'name': 'd'},
                {'#': ids[4], 'name': 'e'},
            ])
            res = await client.query("[.a1, .a2, .a3];")
            self.assertEqual(res, [
                [2, 3, 4],
                [3, 4, 5],
                [4, 5, 6],
            ])

        await client0.query(r"""//ti
            a = .myarr;
            wse(a.pop());
        """)


if __name__ == '__main__':
    run_test(TestNested())

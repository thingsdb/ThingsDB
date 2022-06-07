#!/usr/bin/env python
import asyncio
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client
from thingsdb.exceptions import AuthError
from thingsdb.exceptions import ForbiddenError
from thingsdb.exceptions import BadDataError


class TestGC(TestBase):

    title = 'Test garbage collection'

    @default_test_setup(num_nodes=2, seed=1, threshold_full_storage=50)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        stuff = '@:stuff'

        await client.query(r'''
            .a = {};
            .a.other = {theanswer: 42, ref: .a};
            .x = .a.other;
        ''', scope=stuff)

        await client.query(r'''
            .b = {name: 'Iris'};
            .b.me = .b;
            .del('b');
        ''', scope=stuff)

        other_id = await client.query(r'''
            .other = {};

            // test nested things
            tmp = {
                other: .other,
                a: {
                    other: .other
                }
            };
            tmp.a.me = tmp.a;
            tmp.me = tmp;

            // test cross-refferences in block scopes
            {
                arr = [{other: .other}, {other: .other}];
                arr[0].t1 = arr[1];
                arr[1].t0 = arr[0];
            };

            // test as closure arguments
            fun = |x| {
                x.me = x;
                nil;
            };
            fun.call({other: .other});

            id = .other.id();
            .del('other');
            id;
        ''', scope=stuff)

        with self.assertRaisesRegex(
                LookupError,
                r'collection `stuff` has no `thing` with id [0-9]+'):
            await client.query(f'thing({other_id});', scope=stuff)

        await self.node0.shutdown()
        await self.node0.run()

        await asyncio.sleep(4)

        for _ in range(10):
            await client.query(r'''.counter = 1;''', scope=stuff)

        await self.node0.shutdown()
        await self.node0.run()

        await asyncio.sleep(4)

        x, other = await client.query(
            r'return [.x, .a.other], 2;', scope=stuff)
        self.assertEqual(x['theanswer'], 42)
        self.assertEqual(x, other)

        await client.query(r'''
            .c = {arr: [{name: 'Iris'}]};
            .c.arr.push(.c);
            .c.arr.push({name: 'Cato'});
            .del('c');
        ''', scope=stuff)

        await client.query(r'''
            .xx = {};
            .yy = {};
            .xx.yy = .yy;
            .yy.xx = .xx;

            .del('xx');
            .del('yy');
        ''', scope=stuff)

        await client.query(r'''
            new_type('W');

            .w = {}.wrap('W');
            .w.unwrap().w = .w;

            .del('w');
        ''', scope=stuff)

        await client.query(r'''
            .n = {};
            .del('a');
            .del('x');
        ''', scope=stuff)

        # add another node so away node and gc is forced
        await self.node1.join_until_ready(client)

        for i in range(2):
            await client.query('.x = i;', i=i, scope=stuff)
            await self.wait_nodes_stored()

        counters = await client.query('counters();', scope='@node')

        self.assertEqual(counters['garbage_collected'], 9)

        # below do an advanced garbage collection test
        client.set_default_scope(stuff)

        n = 20
        ids = await client.query(r'''
            things = range(n).map(|| {});
            things.each(|t| {
                t.me = t;
                t.other = {};
                t.garbage = {};
                t.garbage.me = t.garbage;
                t.ref = {};
                t.ref.parent = t;
            });
            .a = things;
            .b = [];
            .a.map(|t| t.id());
        ''', n=n)

        b = []

        for x, id in enumerate(ids):
            await client.query(r'''
                .a.shift();
            ''')

            await asyncio.sleep(x % 15)

            if await client.query(r'''
                try({
                    t = thing(id);
                    t.id = id;
                    .b.push(t);
                    return true;
                });
                false;
            ''', id=id):
                b.append(id)

        await self.node1.shutdown()
        await self.node1.run()

        await asyncio.sleep(4)

        await self.wait_nodes_ready()

        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')

        for t in b:
            for cl in (client, client1):
                self.assertTrue(await cl.query(r'''
                    is_thing(thing(id));
                ''', id=t))

        client1.close()
        await client1.wait_closed()

        await client.query(r'''
            .del('b');
        ''')

        await asyncio.sleep(2)

        await self.node1.shutdown()
        await self.node1.run()
        await asyncio.sleep(4)
        await self.wait_nodes_ready()

        for val in range(50):
            await client.query(r'''
                .val = val;
            ''', val=val)

        await self.node1.shutdown()
        await self.node1.run()
        await asyncio.sleep(4)
        await self.wait_nodes_ready()


if __name__ == '__main__':
    run_test(TestGC())

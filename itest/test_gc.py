#!/usr/bin/env python
import asyncio
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client
from thingsdb.exceptions import AuthError
from thingsdb.exceptions import ForbiddenError
from thingsdb.exceptions import BadRequestError
from thingsdb import scope


class TestGC(TestBase):

    title = 'Test garbage collection'

    @default_test_setup(num_nodes=2, seed=1)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        stuff = scope.Scope('stuff')

        await client.query(r'''
            a = {};
            a.other = {theanswer: 42, ref: a};
            x = a.other;
        ''', target=stuff)

        await client.query(r'''
            b = {name: 'Iris'};
            b.me = b;
            del('b');
        ''', target=stuff)

        await self.node0.shutdown()
        await self.node0.run()

        await asyncio.sleep(4)

        for _ in range(10):
            await client.query(r'''counter = 1;''', target=stuff)

        await self.node0.shutdown()
        await self.node0.run()

        await asyncio.sleep(4)

        x, other = await client.query(r'x; a.other;', target=stuff, all_=True)
        self.assertEqual(x['theanswer'], 42)
        self.assertEqual(x, other)

        await client.query(r'''
            n = {};
            del('a');
        ''', target=stuff)

        # add another node so away node and gc is forced
        await self.node1.join_until_ready(client)

        counters = await client.query('counters();', target=scope.node)
        self.assertEqual(counters['garbage_collected'], 1)


if __name__ == '__main__':
    run_test(TestGC())

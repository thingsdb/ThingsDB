#!/usr/bin/env python
import asyncio
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client
from thingsdb.exceptions import AuthError
from thingsdb.exceptions import ForbiddenError
from thingsdb.exceptions import BadRequestError


class TestGC(TestBase):

    title = 'Test garbage collection'

    @default_test_setup(num_nodes=1, seed=1)
    async def run(self):

        await self.node0.init_and_run()

        with self.assertRaisesRegex(AuthError, 'invalid username or password'):
            await get_client(self.node0, username='test', password='test')

        client = await get_client(self.node0)

        await client.query(r'''new_collection("stuff");''')

        # #3: [
        #   new: {
        #       theanswer: 42,
        #       ref: #2,
        #   }
        # ]
        # #2: [
        #   new: {},
        #   assign('other', #3)
        # ]
        # #0: [
        #   assign('a', {#2});
        #   assign('x', {#3});
        # ]

        await client.query(r'''
            a = {};
            a.other = {theanswer: 42, ref: a};
            x = a.other;
        ''', target='stuff')

        # #2: [
        #   assign('name', 'Iris')
        #

        await client.query(r'''
            b = {name: 'Iris'};
            b.me = b;
            del('b');
        ''', target='stuff')

        await self.node0.shutdown()
        await self.node0.run()

        await asyncio.sleep(3)

        x = await client.query(r'''x;''', target='stuff')
        print('X: ', x)
        self.assertEqual(x.prop['theanswer'], 42)

if __name__ == '__main__':
    run_test(TestGC())

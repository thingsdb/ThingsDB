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


class TestArguments(TestBase):

    title = 'Test arguments'

    @default_test_setup(num_nodes=1, seed=1)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.use('stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_invalid(self, client):
        with self.assertRaisesRegex(
                LookupError,
                r'thing `#1` not found; if you want to create a new thing '
                r'then remove the id \(`#`\) and try again '
                r'\(in argument `blob`\)'):
            await client.query(r'blob;', blob={'#': 1})

        with self.assertRaisesRegex(
                TypeError,
                r'property names must be of type `str` and follow the '
                r'naming rules'):
            await client.query(r'blob;', blob={0: 1})

        with self.assertRaisesRegex(
                LookupError,
                r'thing `#1` not found; if you want to create a new thing '
                r'then remove the id \(`#`\) and try again '
                r'\(in argument `blob`\)'):
            await client.query(r'blob;', blob=[{'#': 1}])

    async def test_valid(self, client):
        iris = {
            'name': 'Iris',
            'age': 6,
            'likes': ['swimming', 'cycling'],
            'friends': [{
                'name': 'Cato',
                'age': 5.5,
                'blonde': True
            }]
        }
        self.assertEqual(
            await client.query('return(iris, 2);', iris=iris),
            iris)

        await client.query('.iris = iris;', iris=iris)
        Iris = await client.query('.iris')
        Cato = await client.query('.iris.friends[0];')
        self.assertEqual(Iris['name'], 'Iris')
        self.assertEqual(Cato['name'], 'Cato')
        self.assertIn('#', Cato)
        self.assertIn('#', Iris)

    async def test_multiple(self, client):
        args = {
            'i': 42,
            'd': 3.14,
            'n': None,
            't': True,
            'f': False,
        }
        await client.query(r'''
            .i = i;
            .d = d;
            .n = n;
            .t = t;
            .f = f;
        ''', **args)

        self.assertEqual(await client.query('.filter(||true);'), args)


if __name__ == '__main__':
    run_test(TestArguments())

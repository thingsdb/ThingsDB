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


class TestOperators(TestBase):

    title = 'Test operators'

    @default_test_setup(num_nodes=1, seed=1)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.use('stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_ternary(self, client):
        """Make sure we do this better than PHP ;-)"""
        self.assertEqual(await client.query(r'''
           (false) ? 'a' : (false) ? 'b' : 'c';
        '''), 'c')

        self.assertEqual(await client.query(r'''
           (false) ? 'a' : (true) ? 'b' : 'c';
        '''), 'b')

        self.assertEqual(await client.query(r'''
           (true) ? 'a' : (false) ? 'b' : 'c';
        '''), 'a')

        self.assertEqual(await client.query(r'''
           (true) ? 'a' : (true) ? 'b' : 'c';
        '''), 'a')

        self.assertEqual(await client.query(r'''
           $initial = 'J';
           $name = ($initial == 'M') ? 'Mike'
            : ($initial == 'J') ? 'John'
            : ($initial == 'C') ? 'Catherina'
            : ($initial == 'T') ? 'Thomas'
            : 'unknown';
        '''), 'John')

    async def test_logical_operators(self, client):
        self.assertTrue(await client.query('(true && true)'))
        self.assertFalse(await client.query('(true && false)'))
        self.assertFalse(await client.query('(false && true)'))
        self.assertFalse(await client.query('(false && false)'))

        self.assertTrue(await client.query('(true || true)'))
        self.assertTrue(await client.query('(true || false)'))
        self.assertTrue(await client.query('(false || true)'))
        self.assertFalse(await client.query('(false || false)'))

        self.assertTrue(await client.query('(1 || 0 && 0)'))
        self.assertFalse(await client.query('((1 || 0) && 0)'))

    async def test_not_operator(self, client):
        self.assertTrue(await client.query('!false'))
        self.assertFalse(await client.query('!true'))

        self.assertFalse(await client.query('!!false'))
        self.assertTrue(await client.query('!!true'))


if __name__ == '__main__':
    run_test(TestOperators())

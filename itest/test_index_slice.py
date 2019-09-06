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


class TestIndexSlice(TestBase):

    title = 'Test index and slices'

    @default_test_setup(num_nodes=1, seed=1)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.use('stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_illigal(self, client):
        s = 'abcdef'
        tu = [1, 2, 3, 4, 5, 6]
        await client.query(f'.raw = "{s}";')
        await client.query(f'.list = [{tu}];')

        with self.assertRaisesRegex(
                BadDataError,
                r'type `raw` does not support index assignments'):
            await client.query('.raw[0] = "z";')

        with self.assertRaisesRegex(
                BadDataError,
                r'type `tuple` does not support index assignments'):
            await client.query('.list[0][0] = "z";')

    async def test_from_raw_by_index(self, client):
        s = 'abcdef'
        n = len(s)
        await client.query(f'.raw = "{s}";')

        for i in range(-n, n):
            self.assertEqual(await client.query(f'.raw[{i}]'), s[i])

        with self.assertRaisesRegex(
                IndexError,
                'index out of range'):
            await client.query(f'.raw[{n}];')

        with self.assertRaisesRegex(
                IndexError,
                'index out of range'):
            await client.query(f'.raw[{-(n+1)}];')

        with self.assertRaisesRegex(
                BadDataError,
                r'expecting an index of type `int` '
                r'but got type `nil` instead'):
            await client.query(f'.raw[nil];')

    async def test_from_raw_by_slice(self, client):
        s = 'abcdef'
        n = len(s)
        await client.query(f'.raw = "{s}";')

        self.assertTrue(await client.query('(.raw[] == .raw);'))
        self.assertTrue(await client.query('(.raw[:] == .raw);'))
        self.assertTrue(await client.query('(.raw[::] == .raw);'))
        self.assertTrue(await client.query('(.raw[::] == .raw);'))
        self.assertTrue(await client.query('(.raw[nil:nil:nil] == .raw);'))
        self.assertTrue(await client.query('(.raw[0::] == .raw);'))
        self.assertTrue(await client.query('(.raw[-100:100:1] == .raw);'))

        self.assertEqual(await client.query('.raw[-3:]'), s[-3:])
        self.assertEqual(await client.query('.raw[:-3]'), s[:-3])
        self.assertEqual(await client.query('.raw[:4:]'), s[:4:])
        self.assertEqual(await client.query('.raw[-3:-1]'), s[-3:-1])
        self.assertEqual(await client.query('.raw[::-1]'), s[::-1])
        self.assertEqual(await client.query('.raw[::2]'), s[::2])
        self.assertEqual(await client.query('.raw[1::2]'), s[1::2])

    async def test_from_arr_by_index(self, client):
        li = [1, 2, 3, 4, 5, 6]
        n = len(li)
        await client.query(f'.list = {li};')

        for i in range(-n, n):
            self.assertEqual(await client.query(f'.list[{i}]'), li[i])

        with self.assertRaisesRegex(
                IndexError,
                'index out of range'):
            await client.query(f'.list[{n}];')

        with self.assertRaisesRegex(
                IndexError,
                'index out of range'):
            await client.query(f'.list[{-(n+1)}];')

        with self.assertRaisesRegex(
                BadDataError,
                r'expecting an index of type `int` '
                r'but got type `nil` instead'):
            await client.query(f'.list[nil];')

    async def test_from_arr_by_slice(self, client):
        li = [1, 2, 3, 4, 5, 6]
        n = len(li)
        await client.query(f'.list = {li};')

        self.assertTrue(await client.query('(.list[] == .list);'))
        self.assertTrue(await client.query('(.list[:] == .list);'))
        self.assertTrue(await client.query('(.list[::] == .list);'))
        self.assertTrue(await client.query('(.list[::] == .list);'))
        self.assertTrue(await client.query('(.list[nil:nil:nil] == .list);'))
        self.assertTrue(await client.query('(.list[0::] == .list);'))
        self.assertTrue(await client.query('(.list[-100:100:1] == .list);'))

        self.assertEqual(await client.query('.list[-3:]'), li[-3:])
        self.assertEqual(await client.query('.list[:-3]'), li[:-3])
        self.assertEqual(await client.query('.list[:4:]'), li[:4:])
        self.assertEqual(await client.query('.list[-3:-1]'), li[-3:-1])
        self.assertEqual(await client.query('.list[::-1]'), li[::-1])
        self.assertEqual(await client.query('.list[::2]'), li[::2])
        self.assertEqual(await client.query('.list[1::2]'), li[1::2])

    async def test_set_arr_by_index(self, client):
        li = [1, 2, 3, 4, 5, 6]
        to = ['a', 'b', 'c', 'd', 'e', 'f']
        n = len(li)
        await client.query(f'.list = {li};')

        for i, c in enumerate(to):
            self.assertEqual(await client.query(f'.list[{i}] = "{c}";'), c)

        self.assertEqual(await client.query('.list'), to)


if __name__ == '__main__':
    run_test(TestIndexSlice())

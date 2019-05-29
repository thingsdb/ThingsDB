#!/usr/bin/env python
import asyncio
import pickle
import time
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client
from thingsdb.target import Target
from thingsdb.exceptions import AssertionError
from thingsdb.exceptions import BadRequestError
from thingsdb.exceptions import IndexError
from thingsdb.exceptions import OverflowError
from thingsdb.exceptions import ZeroDivisionError


class TestThingsDBFunctions(TestBase):

    title = 'Test ThingsDB functions'

    @default_test_setup(num_nodes=1, seed=1)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_collection(self, client):
        with self.assertRaisesRegex(
                BadRequestError,
                'function `collection` takes 1 argument but 0 were given'):
            await client.query('collection();')

        with self.assertRaisesRegex(
                BadRequestError,
                'expecting type `raw` or `int` as collection '
                'but got type `nil` instead'):
            await client.query('collection(nil);')

        with self.assertRaisesRegex(IndexError, 'collection `yes!` not found'):
            await client.query('collection("yes!");')

        with self.assertRaisesRegex(IndexError, '`collection:0` not found'):
            await client.query('collection(0);')

        with self.assertRaisesRegex(IndexError, r'`collection:\d+` not found'):
            await client.query('collection(-1);')

        collection = await client.query('collection("stuff");')
        self.assertTrue(isinstance(collection, dict))
        self.assertEqual(
            await client.query(f'collection({collection["collection_id"]});'),
            collection)

    async def test_collections(self, client):
        with self.assertRaisesRegex(
                BadRequestError,
                'function `collections` takes 0 arguments but 1 was given'):
            await client.query('collections(nil);')

        collections = await client.query('collections();')
        self.assertEqual(len(collections), 1)
        self.assertIn("collection_id", collections[0])
        self.assertIn("name", collections[0])
        self.assertIn("things", collections[0])
        self.assertIn("quota_things", collections[0])
        self.assertIn("quota_properties", collections[0])
        self.assertIn("quota_array_size", collections[0])
        self.assertIn("quota_raw_size", collections[0])

    async def test_counters(self, client):
        with self.assertRaisesRegex(
                BadRequestError,
                'function `counters` takes 0 arguments but 1 was given'):
            await client.query('counters(nil);')


if __name__ == '__main__':
    run_test(TestThingsDBFunctions())

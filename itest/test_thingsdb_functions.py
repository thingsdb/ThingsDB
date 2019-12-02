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


class TestThingsDBFunctions(TestBase):

    title = 'Test thingsdb scope functions'

    @default_test_setup(num_nodes=1, seed=1)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_unknown(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'function `unknown` is undefined'):
            await client.query('unknown();')

        with self.assertRaisesRegex(
                LookupError,
                'function `node_info` is undefined in the `@thingsdb` scope; '
                'you might want to query a `@node` scope?'):
            await client.query('node_info();')

        with self.assertRaisesRegex(
                LookupError,
                'the `root` of the `@thingsdb` scope is inaccessible; '
                'you might want to query a `@collection` scope?'):
            await client.query('.v = 1;')

    async def test_collection_info(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `collection_info` takes 1 argument '
                'but 0 were given'):
            await client.query('collection_info();')

        with self.assertRaisesRegex(
                TypeError,
                'expecting type `str` or `int` as collection '
                'but got type `list` instead'):
            await client.query('collection_info([]);')

        with self.assertRaisesRegex(
                LookupError,
                'collection `yes!` not found'):
            await client.query('collection_info("yes!");')

        with self.assertRaisesRegex(
                LookupError,
                '`collection:0` not found'):
            await client.query('collection_info(0);')

        with self.assertRaisesRegex(
                LookupError,
                r'`collection:\d+` not found'):
            await client.query('collection_info(-1);')

        collection = await client.query('collection_info("stuff");')
        self.assertTrue(isinstance(collection, dict))
        self.assertEqual(
            await client.query(
                f'collection_info({collection["collection_id"]});'),
            collection)

    async def test_collections_info(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `collections_info` takes 0 arguments '
                'but 1 was given'):
            await client.query('collections_info(nil);')

        collections = await client.query('collections_info();')
        self.assertEqual(len(collections), 1)
        self.assertEqual(len(collections[0]), 7)

        self.assertIn("collection_id", collections[0])
        self.assertIn("name", collections[0])
        self.assertIn("things", collections[0])
        self.assertIn("quota_things", collections[0])
        self.assertIn("quota_properties", collections[0])
        self.assertIn("quota_array_size", collections[0])
        self.assertIn("quota_raw_size", collections[0])

        self.assertTrue(isinstance(collections[0]["collection_id"], int))
        self.assertTrue(isinstance(collections[0]["name"], str))
        self.assertTrue(isinstance(collections[0]["things"], int))
        self.assertIs(collections[0]["quota_things"], None)
        self.assertIs(collections[0]["quota_properties"], None)
        self.assertIs(collections[0]["quota_array_size"], None)
        self.assertIs(collections[0]["quota_raw_size"], None)

    async def test_del_collection(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `del_collection` takes 1 argument '
                'but 0 were given'):
            await client.query('del_collection();')

        with self.assertRaisesRegex(
                TypeError,
                'expecting type `str` or `int` as collection '
                'but got type `float` instead'):
            await client.query('del_collection(1.0);')

        with self.assertRaisesRegex(
                LookupError,
                'collection `A` not found'):
            await client.query('del_collection("A");')

        with self.assertRaisesRegex(
                LookupError,
                '`collection:1234` not found'):
            await client.query('del_collection(1234);')

        test1 = await client.query('new_collection("test1");')
        test2 = await client.query('new_collection("test2");')

        self.assertIs(await client.query('del_collection("test1");'), None)
        self.assertIs(await client.query(f'del_collection({test2});'), None)

    async def test_del_user(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `del_user` takes 1 argument but 0 were given'):
            await client.query('del_user();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `del_user` expects argument 1 to be of type `str` '
                r'but got type `int` instead'):
            await client.query('del_user(42);')

        with self.assertRaisesRegex(
                ValueError,
                'user name must follow the naming rules'):
            await client.query('del_user("");')

        with self.assertRaisesRegex(
                OperationError,
                'it is not possible to delete your own user account'):
            await client.query('del_user("admin");')

        await client.query('new_user("iris");')

        self.assertIs(await client.query('del_user("iris");'), None)

    async def test_grant(self, client):
        await client.query('new_user("iris");')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `grant` takes 3 arguments but 0 were given'):
            await client.query('grant();')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid scope; '
                r'scopes must start with a `@` or `/` but got `A` instead'):
            await client.query('grant("A", "x", FULL);')

        with self.assertRaisesRegex(LookupError, 'collection `A` not found'):
            await client.query('grant("@:A", "x", FULL);')

        with self.assertRaisesRegex(
                TypeError,
                r'function `grant` expects argument 2 to be of type `str` '
                r'but got type `nil` instead'):
            await client.query('grant("@:stuff", nil, FULL);')

        with self.assertRaisesRegex(LookupError, 'user `X` not found'):
            await client.query('grant("@:stuff", "X", FULL);')

        with self.assertRaisesRegex(
                TypeError,
                r'function `grant` expects argument 3 to be of type `int` '
                r'but got type `nil` instead'):
            await client.query('grant("@:stuff", "iris", nil);')

        await client.query('del_user("iris");')

    async def test_rename_collection(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `rename_collection` takes 2 arguments '
                'but 0 were given'):
            await client.query('rename_collection();')

        with self.assertRaisesRegex(
                TypeError,
                'expecting type `str` or `int` as collection '
                'but got type `float` instead'):
            await client.query('rename_collection(1.0, "bla");')

        with self.assertRaisesRegex(
                ValueError,
                'collection name must follow the naming rules'):
            await client.query('rename_collection("stuff", "4bla");')

        with self.assertRaisesRegex(
                LookupError,
                'collection `A` not found'):
            await client.query('rename_collection("A", "B");')

        with self.assertRaisesRegex(
                LookupError,
                '`collection:1234` not found'):
            await client.query('rename_collection(1234, "B");')

        test = await client.query('new_collection("test1");')
        self.assertIs(
            await client.query('rename_collection("test1", "test2");'),
            None)

        self.assertIs(await client.query('del_collection("test2");'), None)


if __name__ == '__main__':
    run_test(TestThingsDBFunctions())

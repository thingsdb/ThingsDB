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
from thingsdb.exceptions import ForbiddenError


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
        await client.query('''
            new_user('user1');
            set_password('user1', 'pass1');
            grant('@t', 'user1', QUERY);
            grant('@n', 'user1', JOIN);
        ''', scope='@t')

        user1_cl = await get_client(self.node0, auth=['user1', 'pass1'])

        with self.assertRaisesRegex(
                ForbiddenError,
                r'user `user1` is missing the required privileges '
                r'\(`QUERY|RUN`\) on scope `@collection:stuff`'):
            await user1_cl.query('collection_info("stuff");')

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
        now = time.time()
        await client.query('''
            new_user('user2');
            set_password('user2', 'pass2');
            grant('@t', 'user2', QUERY);
            grant('@n', 'user2', JOIN);
        ''', scope='@t')

        user1_cl = await get_client(self.node0, auth=['user2', 'pass2'])

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `collections_info` takes 0 arguments '
                'but 1 was given'):
            await client.query('collections_info(nil);')

        collections = await user1_cl.query('collections_info();')
        self.assertEqual(len(collections), 0)

        collections = await client.query('collections_info();')
        self.assertEqual(len(collections), 1)
        self.assertEqual(len(collections[0]), 7)

        self.assertIn("collection_id", collections[0])
        self.assertIn("next_free_id", collections[0])
        self.assertIn("name", collections[0])
        self.assertIn("things", collections[0])
        self.assertIn("created_at", collections[0])
        self.assertIn("time_zone", collections[0])
        self.assertIn("default_deep", collections[0])

        self.assertTrue(isinstance(collections[0]["collection_id"], int))
        self.assertTrue(isinstance(collections[0]["next_free_id"], int))
        self.assertTrue(isinstance(collections[0]["name"], str))
        self.assertTrue(isinstance(collections[0]["things"], int))
        self.assertTrue(isinstance(collections[0]["created_at"], int))
        self.assertTrue(isinstance(collections[0]["time_zone"], str))
        self.assertTrue(isinstance(collections[0]["default_deep"], int))

        # at least one info should be checked for a correct created_at info
        self.assertGreater(collections[0]['created_at'], now - 60)
        self.assertLess(collections[0]['created_at'], now + 60)

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
        self.assertIs(
            await client.query('del_collection(test1);', test1=test1), None)

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

    async def test_has_collection(self, client):
        await client.query(r'''new_collection('Ti')''')

        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `has_collection`'):
            await client.query('nil.has_collection();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `has_collection` takes 1 argument but 0 were given'):
            await client.query('has_collection();')

        with self.assertRaisesRegex(
                TypeError,
                r'expecting type `str` or `int` as collection '
                r'but got type `nil` instead'):
            await client.query('has_collection(nil);')

        self.assertTrue(await client.query(r'''has_collection('Ti');'''))
        self.assertFalse(await client.query(r'''has_collection('ti');'''))

    async def test_has_node(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `has_node`'):
            await client.query('nil.has_node();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `has_node` takes 1 argument but 2 were given'):
            await client.query('has_node(1, 2);')

        with self.assertRaisesRegex(
                TypeError,
                r'function `has_node` expects argument 1 to be of '
                r'type `int` but got type `str` instead'):
            await client.query('has_node("@n");')

        with self.assertRaisesRegex(
                ValueError,
                r'function `has_node` expects argument 1 to be an '
                r'integer value between 0 and 4294967295'):
            await client.query('has_node(-1);')

        self.assertTrue(await client.query(r'''has_node(0);'''))
        self.assertFalse(await client.query(r'''has_node(42);'''))

    async def test_has_token(self, client):
        token = await client.query('''
            new_user('has_token');
            set_password('has_token', 'pass');
            grant('@t', 'has_token', QUERY);
            grant('@n', 'has_token', JOIN);
            new_token('admin');
        ''', scope='@t')

        cl = await get_client(self.node0, auth=['has_token', 'pass'])

        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `has_token`'):
            await client.query('nil.has_token();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `has_token` takes 1 argument but 2 were given'):
            await client.query('has_token(1, 2);')

        with self.assertRaisesRegex(
                TypeError,
                r'function `has_token` expects argument 1 to be of '
                r'type `str` but got type `int` instead'):
            await client.query('has_token(123);')

        with self.assertRaisesRegex(
                ValueError,
                r'function `has_token` expects argument 1 to be token string'):
            await client.query('has_token("invalid");')

        with self.assertRaisesRegex(
                ForbiddenError,
                r'user `has_token` is missing the required '
                r'privileges \(`GRANT`\) on scope `@thingsdb`'):
            await cl.query('has_token("stuff");')

        self.assertTrue(await client.query(
            f'''has_token("{token}");'''))

        self.assertFalse(await client.query(
            f'''has_token("{'X' * len(token)}");'''))

    async def test_has_user(self, client):
        await client.query('''
            new_user('has_user');
            set_password('has_user', 'pass');
            grant('@t', 'has_user', QUERY);
            grant('@n', 'has_user', JOIN);
        ''', scope='@t')

        cl = await get_client(self.node0, auth=['has_user', 'pass'])

        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `has_user`'):
            await client.query('nil.has_user();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `has_user` takes 1 argument but 2 were given'):
            await client.query('has_user(1, 2);')

        with self.assertRaisesRegex(
                TypeError,
                r'function `has_user` expects argument 1 to be of '
                r'type `str` but got type `int` instead'):
            await client.query('has_user(123);')

        with self.assertRaisesRegex(
                ForbiddenError,
                r'user `has_user` is missing the required '
                r'privileges \(`GRANT`\) on scope `@thingsdb`'):
            await cl.query('has_token("stuff");')

        self.assertTrue(await client.query(f'''has_user("has_user");'''))
        self.assertFalse(await client.query(f'''has_user("XX");'''))

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

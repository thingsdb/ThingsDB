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


class TestScopes(TestBase):

    title = 'Test scope naming'

    @default_test_setup(num_nodes=1, seed=1)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_scope_errors(self, client):
        with self.assertRaisesRegex(
                ValueError,
                r'invalid scope; scopes must not be empty'):
            await client.query(r'''
                grant('', FULL, 'iris');
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid scope; scopes must start with a `@` '
                r'but got `!` instead'):
            await client.query(r'''
                grant('!', FULL, 'iris');
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid scope; expecting a scope name '
                r'like `@thingsdb`, `@node:...` or `@collection:...`'):
            await client.query(r'''
                grant('@', FULL, 'iris');
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid scope; scopes must only contain valid ASCII '
                r'characters'):
            await client.query(r'''
                grant('π', FULL, 'iris');
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid scope; a node id must be an integer value '
                r'between 0 and 64;'):
            await client.query(r'''
                grant('@node:64', FULL, 'iris');
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid scope; a node id must be an integer value '
                r'between 0 and 64;'):
            await client.query(r'''
                grant('@n:1a', FULL, 'iris');
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid scope; a collection must be specified by '
                r'either a name or id;'):
            await client.query(r'''
                grant('@collection:1a', FULL, 'iris');
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid scope; the specified collection name is invalid'):
            await client.query(r'''
                grant('@collection:a!', FULL, 'iris');
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r"invalid scope; expecting a scope name "
                r"like `@thingsdb`, `@node:...` or `@collection:...` "
                r"but got `@collectionn:stuff` instead;"):
            await client.query(r'''
                grant('@collectionn:stuff', FULL, 'iris');
            ''')

    async def test_node_scope(self, client):
        q = 'counters(); true;'
        self.assertTrue(await client.query(q, scope='@n'))
        self.assertTrue(await client.query(q, scope='@no'))
        self.assertTrue(await client.query(q, scope='@node'))
        self.assertTrue(await client.query(q, scope='@node:'))
        self.assertTrue(await client.query(q, scope='@n:0'))
        self.assertTrue(await client.query(q, scope='@node:0'))

    async def test_thingsdb_scope(self, client):
        q = 'users_info(); true;'
        self.assertTrue(await client.query(q, scope='@t'))
        self.assertTrue(await client.query(q, scope='@thing'))
        self.assertTrue(await client.query(q, scope='@thingsdb'))

    async def test_collection_scope(self, client):
        q = '.id(); true;'
        self.assertTrue(await client.query(q, scope='@:stuff'))
        self.assertTrue(await client.query(q, scope='@c:stuff'))
        self.assertTrue(await client.query(q, scope='@col:stuff'))
        self.assertTrue(await client.query(q, scope='@collection:stuff'))


if __name__ == '__main__':
    run_test(TestScopes())

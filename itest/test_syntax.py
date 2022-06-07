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
from thingsdb.exceptions import SyntaxError
from thingsdb.exceptions import TypeError
from thingsdb.exceptions import NumArgumentsError
from thingsdb.exceptions import BadDataError
from thingsdb.exceptions import LookupError
from thingsdb.exceptions import OverflowError
from thingsdb.exceptions import ZeroDivisionError
from thingsdb.exceptions import OperationError


class TestSyntax(TestBase):

    title = 'Test syntax'

    @default_test_setup(num_nodes=1, seed=1, threshold_full_storage=10)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_long_prop(self, client):
        await client.query('.{} = 1'.format('a'*255))

        with self.assertRaisesRegex(
                SyntaxError,
                r'error at line 1, position 1, unexpected character `a`'):
            await client.query('.{} = 1'.format('a'*1000))

        with self.assertRaisesRegex(
                SyntaxError,
                r'error at line 1, position 1, unexpected character `a`'):
            await client.query('.{} = 1'.format('a'*256))

        await client.query('thing(.id())[prop] = 1', prop='b'*255)

        await client.query('thing(.id()).set(prop, 1)', prop='c'*255)

    async def test_invalid_syntax(self, client):
        with self.assertRaisesRegex(
                SyntaxError,
                r'error at line 1, position 17, expecting: :'):
            await client.query('|x| is_err(x)?x+2')

        with self.assertRaisesRegex(
                SyntaxError,
                r'error at line 1, position 17, expecting: :'):
            await client.query('|x| is_err(x)?x+2')

    async def test_weird_closure(self, client):
        with self.assertRaisesRegex(
                SyntaxError,
                r'error at line 1, position 1, unexpected character `\|`.*'):
            await client.query("""//ti
                !||nil;
            """)

    async def test_closure_store(self, client):
        res = await client.query("""//ti
            .fun = || {
                // This is a test
                // with comments on top.
                nil;
            }
            .fun;
        """)
        self.assertEqual(res, """||{// This is a test
// with comments on top.
nil;}""")

        res = await client.query("""//ti
            .fun = || {
                for (x in range(10)) {4}5;
            };
            .fun;
        """)
        self.assertEqual(res, "||{for(x in range(10)){4}5;}")

    async def test_missing_semicolon(self, client):
        res = await client.query("""//ti
            123 456;
        """)
        self.assertEqual(res, 456)

        with self.assertRaisesRegex(
                SyntaxError,
                r'error at line 1, position 0, unexpected character `1`, '
                r'expecting: if, return, for or end_of_statement'):
            await client.query("""//ti
                1.2.3;
            """)


if __name__ == '__main__':
    run_test(TestSyntax())

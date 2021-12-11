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


class TestStatements(TestBase):

    title = 'Test statements'

    @default_test_setup(num_nodes=2, seed=1, threshold_full_storage=10)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        # add another node for query validation
        await self.node1.join_until_ready(client)

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_if_statement(self, client):
        res = await client.query("""//ti
            if (true) {
                42;
            } else {
                0;
            };
        """)
        self.assertEqual(res, 42)

        res = await client.query("""//ti
            if (false) {
                42/0;
            } else {
                0;
            };
        """)
        self.assertEqual(res, 0)

        res = await client.query("""//ti
            if(true)42;
        """)
        self.assertEqual(res, 42)

        res = await client.query("""//ti
            if(false)42;
        """)
        self.assertIs(res, False)

        res = await client.query("""//ti
            if([])42;
        """)
        self.assertEqual(res, [])

        res = await client.query("""//ti
            if(1)42 else 1/0;
        """)
        self.assertEqual(res, 42)

        res = await client.query("""//ti
            x = 42; y = 13;
            if(x)return{x}else{return y;};
        """)
        self.assertEqual(res, 42)

        res = await client.query("""//ti
            x = 0; y = 13;
            if(x)return{x}else{return y;};
        """)
        self.assertEqual(res, 13)

        with self.assertRaisesRegex(
                ZeroDivisionError,
                'division or modulo by zero'):
            await client.query("""//ti
                if(1/0)nil;
            """)

        with self.assertRaisesRegex(
                ZeroDivisionError,
                'division or modulo by zero'):
            await client.query("""//ti
                if(true)1/0;
            """)

        with self.assertRaisesRegex(
                ZeroDivisionError,
                'division or modulo by zero'):
            await client.query("""//ti
                if(false)nil else 1/0;
            """)

    async def test_return(self, client):
        res = await client.query("""//ti
            return 42;
        """)
        self.assertEqual(res, 42)

        res = await client.query("""//ti
            return {a: {b: { c: {}}}};
        """)
        self.assertEqual(res, {"a": {}})

        res = await client.query("""//ti
            return {a: {b: { c: {}}}}, 0;
        """)
        self.assertEqual(res, {})

        res = await client.query("""//ti
            return {a: {b: { c: {}}}}, 2;
        """)
        self.assertEqual(res, {"a": {"b": {}}})

        with self.assertRaisesRegex(
                ZeroDivisionError,
                'division or modulo by zero'):
            await client.query("""//ti
                return 1/0;
            """)

        with self.assertRaisesRegex(
                ZeroDivisionError,
                'division or modulo by zero'):
            res = await client.query("""//ti
                return 1/0, 0;
            """)

        with self.assertRaisesRegex(
                ValueError,
                r'expecting a `deep` value between 0 and 127 '
                r'but got -1 instead'):
            res = await client.query("""//ti
                return nil, -1;
            """)

        with self.assertRaisesRegex(
                TypeError,
                r'expecting `deep` to be of type `int` '
                r'but got type `nil` instead'):
            res = await client.query("""//ti
                return nil, nil;
            """)

    async def test_format(self, client):
        res = await client.query("""//ti
            x = || {
                if(true) 0 else 1;
                if(1){0}else{1};
                if(x)return{x}else{return y;};
                return{1},0;
            };
            .x = x;
            str(x);
        """)
        self.assertEqual(res, """|| {
    if (true) 0 else 1;
    if (1) {
        0;
    } else {
        1;
    };
    if (x) return {
        x;
    } else {
        return y;
    };
    return {
        1;
    }, 0;
}""")
        res = await client.query("""//ti
            new_procedure('x', .x);
            .x;
        """)
        self.assertEqual(
            res,
            "||{if(true)0 else 1;"
            "if(1){0}else{1};"
            "if(x)return{x}else{return y;};"
            "return{1},0;}")

        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')

        await self.wait_nodes_ready(client1)

        res = await client1.query("""//ti
            .x;
        """)
        self.assertEqual(
            res,
            "||{if(true)0 else 1;"
            "if(1){0}else{1};"
            "if(x)return{x}else{return y;};"
            "return{1},0;}")

        res = await client1.query("""
            procedure_info('x').load().definition;
        """)
        self.assertEqual(res, """|| {
    if (true) 0 else 1;
    if (1) {
        0;
    } else {
        1;
    };
    if (x) return {
        x;
    } else {
        return y;
    };
    return {
        1;
    }, 0;
}""")


if __name__ == '__main__':
    run_test(TestStatements())

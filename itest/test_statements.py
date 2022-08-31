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
from thingsdb.exceptions import SyntaxError


class TestStatements(TestBase):

    title = 'Test statements'

    @default_test_setup(num_nodes=2, seed=1, threshold_full_storage=10)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        if hasattr(self, 'node1'):
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

    async def test_for_loop(self, client):
        with self.assertRaisesRegex(
                SyntaxError,
                r'illegal `break` statement; '
                r'no surrounding `for..in` statement'):
            res = await client.query("""//ti
                break;
            """)

        with self.assertRaisesRegex(
                SyntaxError,
                r'illegal `continue` statement; '
                r'no surrounding `for..in` statement'):
            res = await client.query("""//ti
                continue;
            """)

        with self.assertRaisesRegex(
                SyntaxError,
                r'illegal `break` statement; '
                r'no surrounding `for..in` statement'):
            res = await client.query("""//ti
                for(x in range(10)) ||break;
            """)

        with self.assertRaisesRegex(
                SyntaxError,
                r'illegal `continue` statement; '
                r'no surrounding `for..in` statement'):
            res = await client.query("""//ti
                for(x in range(10)) ||continue;
            """)

        res = await client.query("""//ti
            x = 123;
            for(x in range(0)) nil; // x should not change
            x;
        """)
        self.assertEqual(res, 123)

        res = await client.query("""//ti
            for(x in range(10)) if(x==7) break;
            x; // x is set to the last in
        """)
        self.assertEqual(res, 7)

        res = await client.query("""//ti
            for(x in range(10)) if(x==7) break;
            x; // x is set to the last in
        """)
        self.assertEqual(res, 7)

        res = await client.query("""//ti
            for(x, y in ['a', 'b', 'c', 'd']) if(y==2) return x;
            nil; // should not be reached
        """)
        self.assertEqual(res, 'c')

        res = await client.query("""//ti
            for(k, v in {a: 0, b: 1, c: 2, d: 3}) if(v==2) return k;
            nil; // should not be reached
        """)
        self.assertEqual(res, 'c')

        res = await client.query("""//ti
            .a = {a: 0};
            .b = {b: 1};
            c = {c: 2};
            .d = {d: 3};

            for(t, id in set(.a, .b, c, .d)) if(id==nil) return t;
            nil; // should not be reached
        """)
        self.assertEqual(res, {'c': 2})

    async def test_return_from_func(self, client):
        with self.assertRaisesRegex(
                TypeError,
                'expecting `deep` to be of type `int` but got '
                'type `float` instead'):
            await client.query('return nil, 0.0;')

        with self.assertRaisesRegex(
                ValueError,
                r'expecting a `deep` value between 0 and 127 '
                r'but got 200 instead'):
            await client.query('return nil, 200;')

        with self.assertRaisesRegex(
                ValueError,
                r'expecting a `deep` value between 0 and 127 '
                r'but got -2 instead'):
            await client.query('return nil, -2;')

        self.assertIs(await client.query(r'''
            return nil;
            "Not returned";
        '''), None)

        self.assertEqual(await client.query(r'''
            return 42;
            "Not returned";
        '''), 42)

        self.assertEqual(await client.query(r'''
            [0, 1, 2].map(|x| {
                return x + 1;
                0;
            });
        '''), [1, 2, 3])

        self.assertEqual(await client.query(r'''
            (|x| {
                try(return x + 1);
                0;
            }).call(41);
        '''), 42)

        self.assertEqual(await client.query(r'''
            .a = 10;
            .a = return 11;  // Return, so do not overwrite a
        '''), 11)

        self.assertEqual(await client.query('.a'), 10)

    async def test_return_flags(self, client):
        res = await client.query("""//ti
            set_type('A', {
                nested: 'thing'
            });
            .a = A();
            return .a, 2, NO_IDS;
        """)
        self.assertEqual(res, {'nested': {}})

        res = await client.query("""//ti
            return .a, 2, 1;
        """)
        self.assertIn('#', res)
        self.assertIn('#', res['nested'])

        res = await client.query("""//ti
            str(|| return 'value',0,NO_IDS);
        """)
        self.assertEqual(res, "|| return 'value', 0, NO_IDS")

        res = await client.query("""//ti
            || return 'value',0,NO_IDS;
        """)
        self.assertEqual(res, "|| return 'value',0,NO_IDS")

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
}""".replace('  ', '\t'))
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

        res = await client.query("""//ti
            y = || {
                for(x,y in range(10)) {
                    if(x<3)continue else break;
                };
            };
            .y = y;
            str(y);
        """)
        self.assertEqual(res, """|| {
  for (x, y in range(10)) {
    if (x < 3) continue else break;
  };
}""".replace('  ', '\t'))
        res = await client.query("""//ti
            new_procedure('y', .y);
            .y;
        """)
        self.assertEqual(
            res,
            "||{for(x,y in range(10)){if(x<3)continue else break;};}")

        if hasattr(self, 'node1'):
            client1 = await get_client(self.node1)
            client1.set_default_scope('//stuff')

            await self.wait_nodes_ready(client1)
        else:
            client1 = client

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
}""".replace('  ', '\t'))
        res = await client1.query("""//ti
            .y;
        """)
        self.assertEqual(
            res,
            "||{for(x,y in range(10)){if(x<3)continue else break;};}")

        res = await client1.query("""
            procedure_info('y').load().definition;
        """)
        self.assertEqual(res, """|| {
  for (x, y in range(10)) {
    if (x < 3) continue else break;
  };
}""".replace('  ', '\t'))


if __name__ == '__main__':
    run_test(TestStatements())

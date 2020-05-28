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
from thingsdb.exceptions import LookupError
from thingsdb.exceptions import OverflowError


class TestOperators(TestBase):

    title = 'Test operators'

    @default_test_setup(num_nodes=1, seed=1)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_mul(self, client):
        self.assertEqual(await client.query('8 * 0'), 0)

    async def test_within_closure(self, client):
        self.assertIs(await client.query('.a = ||nil; nil;'), None)

    async def test_swap(self, client):
        self.assertEqual(await client.query('2 || 5 - 1'), 2)
        self.assertEqual(await client.query('(2 || 5) - 1'), 1)
        self.assertEqual(await client.query('2 - 2 && 1'), 0)
        self.assertEqual(await client.query('2 - (2 && 1)'), 1)
        self.assertEqual(await client.query('5 - 0 ? 1 : 2;'), 1)
        self.assertEqual(await client.query('2 + 3 * 4 + 1'), 15)
        self.assertEqual(await client.query('2 * 3 + 4 * 2'), 14)
        self.assertTrue(await client.query('2 + 2 * 2 + 2 & 1 == 0 && 2'))
        self.assertTrue(await client.query('2 & 1 == 0 && 2'))
        self.assertTrue(await client.query('2 & 1 == 0'))
        self.assertTrue(await client.query('0 == 2 & 1'))
        self.assertTrue(await client.query('2 && 0 == 2 + 2 * 2 + 2 & 1'))
        self.assertTrue(await client.query('1 && 2 & 2 * 2 + 2 & 1 == 0 && 2'))

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
           initial = 'J';
           name = (initial == 'M') ? 'Mike'
            : (initial == 'J') ? 'John'
            : (initial == 'C') ? 'Catherina'
            : (initial == 'T') ? 'Thomas'
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

    async def test_set_operations(self, client):
        await client.query(r'''
            range(0, 200).each(|x| .set('x' + str(x), {n: x}));
        ''')

        res = await client.query(r'''
            (set(.x1, .x2, .x150) | set(.x1, .x150, .x160)).map(|x| x.n);
        ''')
        self.assertEqual(set(res), {1, 2, 150, 160})

        res = await client.query(r'''
            (set(.x1, .x2, .x150) & set(.x1, .x150, .x160)).map(|x| x.n);
        ''')
        self.assertEqual(set(res), {1, 150})

        res = await client.query(r'''
            (set(.x1, .x2, .x150) - set(.x1, .x150, .x160)).map(|x| x.n);
        ''')
        self.assertEqual(set(res), {2})

        res = await client.query(r'''
            (set(.x1, .x2, .x150) ^ set(.x1, .x150, .x160)).map(|x| x.n);
        ''')
        self.assertEqual(set(res), {2, 160})

        res = await client.query(r'''
            a = set(.x1, .x2, .x150);
            b = set(.x1, .x150, .x160);
            res = (a | b).map(|x| x.n);
            assert (a == set(.x1, .x2, .x150));
            assert (b == set(.x1, .x150, .x160));
            res;
        ''')
        self.assertEqual(set(res), {1, 2, 150, 160})

        res = await client.query(r'''
            a = set(.x1, .x2, .x150);
            b = set(.x1, .x150, .x160);
            res = (a & b).map(|x| x.n);
            assert (a == set(.x1, .x2, .x150));
            assert (b == set(.x1, .x150, .x160));
            res;
        ''')
        self.assertEqual(set(res), {1, 150})

        res = await client.query(r'''
            a = set(.x1, .x2, .x150);
            b = set(.x1, .x150, .x160);
            res = (a - b).map(|x| x.n);
            assert (a == set(.x1, .x2, .x150));
            assert (b == set(.x1, .x150, .x160));
            res;
        ''')
        self.assertEqual(set(res), {2})

        res = await client.query(r'''
            a = set(.x1, .x2, .x150);
            b = set(.x1, .x150, .x160);
            res = (a ^ b).map(|x| x.n);
            assert (a == set(.x1, .x2, .x150));
            assert (b == set(.x1, .x150, .x160));
            res;
        ''')
        self.assertEqual(set(res), {2, 160})

        res = await client.query(r'''
            a = set(.x10, .x20, .x30, .x40, .x50);
            a |= set(.x40, .x41, .x42);
            a.map(|x| x.n);
        ''')
        self.assertEqual(set(res), {10, 20, 30, 40, 41, 42, 50})

        res = await client.query(r'''
            a = set(.x10, .x20, .x30, .x40, .x50);
            a &= set(.x40, .x41, .x42);
            a.map(|x| x.n);
        ''')
        self.assertEqual(set(res), {40})

        res = await client.query(r'''
            a = set(.x10, .x20, .x30, .x40, .x50);
            a -= set(.x40, .x41, .x42);
            a.map(|x| x.n);
        ''')
        self.assertEqual(set(res), {10, 20, 30, 50})

        res = await client.query(r'''
            a = set(.x10, .x20, .x30, .x40, .x50);
            a ^= set(.x40, .x41, .x42);
            a.map(|x| x.n);
        ''')
        self.assertEqual(set(res), {10, 20, 30, 41, 42, 50})

        res = await client.query(r'''
            a = set(.x10, .x20, .x30, .x40, .x50);
            b = set(.x40, .x41, .x42);
            a |= b;
            res = a.map(|x| x.n);
            assert (b == set(.x40, .x41, .x42));
            res;
        ''')
        self.assertEqual(set(res), {10, 20, 30, 40, 41, 42, 50})

        res = await client.query(r'''
            a = set(.x10, .x20, .x30, .x40, .x50);
            b = set(.x40, .x41, .x42);
            a &= b;
            res = a.map(|x| x.n);
            assert (b == set(.x40, .x41, .x42));
            res;
        ''')
        self.assertEqual(set(res), {40})

        res = await client.query(r'''
            a = set(.x10, .x20, .x30, .x40, .x50);
            b = set(.x40, .x41, .x42);
            a -= b;
            res = a.map(|x| x.n);
            assert (b == set(.x40, .x41, .x42));
            res;
        ''')
        self.assertEqual(set(res), {10, 20, 30, 50})

        res = await client.query(r'''
            a = set(.x10, .x20, .x30, .x40, .x50);
            b = set(.x40, .x41, .x42);
            a ^= b;
            res = a.map(|x| x.n);
            assert (b == set(.x40, .x41, .x42));
            res;
        ''')
        self.assertEqual(set(res), {10, 20, 30, 41, 42, 50})

    async def test_preopr(self, client):
        self.assertIs(await client.query(r'''
            !true;
        '''), False)
        self.assertIs(await client.query(r'''
            !!true;
        '''), True)
        self.assertIs(await client.query(r'''
            !! 42;
        '''), True)
        self.assertEqual(await client.query(r'''
            -!! 42;
        '''), -1)
        self.assertEqual(await client.query(r'''
            -!!-! 42;
        '''), 0)
        self.assertEqual(await client.query(r'''
            --3.14;
        '''), 3.14)
        self.assertEqual(await client.query(r'''
            -true;
        '''), -1)
        self.assertEqual(await client.query(r'''
            ++true;
        '''), 1)
        self.assertEqual(await client.query(r'''
            +false;
        '''), 0)
        self.assertEqual(await client.query(r'''
            (|x| - ! x).def();
        '''), "|x| -!x")


if __name__ == '__main__':
    run_test(TestOperators())

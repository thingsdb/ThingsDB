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


class TestTypes(TestBase):

    title = 'Test thingsdb types'

    @default_test_setup(num_nodes=1, seed=1)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.use('stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_regex(self, client):
        with self.assertRaisesRegex(
                BadDataError,
                r'cannot compile regular expression \'/invalid\(regex/\', '
                r'missing closing parenthesis'):
            await client.query(r'r = /invalid(regex/;')

    async def test_raw(self, client):
        self.assertEqual(await client.query(r'''
            "Hi ""Iris""!!";
        '''), 'Hi "Iris"!!')
        self.assertEqual(await client.query(r'''
            'Hi ''Iris''!!';
        '''), "Hi 'Iris'!!")
        self.assertTrue(await client.query(r'''
            ("Hi ""Iris""" == 'Hi "Iris"' && 'Hi ''Iris''!' == "Hi 'Iris'!")
        '''))
        self.assertEqual(await client.query(r'''
            blob(0);
        ''', blobs=["Hi 'Iris'!!"]), "Hi 'Iris'!!")
        self.assertTrue(await client.query(' ("Hello"[0] == "H") '))
        self.assertTrue(await client.query(' ("Hello"[0][-1] == "H") '))
        self.assertTrue(await client.query(' ("Hello"[-1] == "o") '))
        self.assertTrue(await client.query(' ("Hello"[-4] == "e") '))
        self.assertTrue(await client.query(' ("Hello" == "Hello") '))
        self.assertTrue(await client.query(' ("Hello" != "hello") '))
        self.assertTrue(await client.query(' ("Hello" != "Hello.") '))

    async def test_thing(self, client):
        self.assertEqual(await client.query(r'''
            [{}.id(), [{}][0].id()];
        '''), [0, 0])

        self.assertEqual(await client.query(r'''
            tmp = {a: [{}], b: {}};
            [tmp.a[0].id(), tmp.b.id(), tmp.id()];
        '''), [0, 0, 0])

        self.assertEqual(await client.query(r'''
           {t: 0}.t;
        '''), 0)

        self.assertEqual(await client.query(r'''
           return({a: {t: 0}}, 0);
        '''), {})

        self.assertEqual(await client.query(r'''
           return({a: {t: 0}}, 1);
        '''), {'a': {}})

        self.assertEqual(await client.query(r'''
           return({a: {t: 0}}, 2);
        '''), {'a': {'t': 0}})

        self.assertGreater(await client.query(r'''
            tmp = {t: {}};
            .t = tmp.t;
            tmp.t.id();
        '''), 0)

        self.assertGreater(await client.query(r'''
            .t = {};
            .t.id();
        '''), 0)

        self.assertGreater(await client.query(r'''
            .a = [{}];
            .a[0].id();
        '''), 0)

        self.assertGreater(await client.query(r'''
            .a = [[{}]];
            .a[0][0].id();
        '''), 0)

        self.assertGreater(await client.query(r'''
            .t = {t: {}};
            .t.t.id();
        '''), 0)

        self.assertGreater(await client.query(r'''
            .t = {l: []};
            .t.l.push({a: {}});
            .t.l[0].id();
        '''), 0)

        self.assertGreater(await client.query(r'''
            .t.l[0].a.id();
        '''), 0)

        self.assertEqual(await client.query(r'''
            t = {l: []};
            t.l.push({a: {}});
            [t.l[0].id(), t.l[0].a.id()];
        '''), [0, 0])

    async def test_array(self, client):
        self.assertEqual(await client.query(r'''
            ({}.t = [1, 2, 3]).push(4);
        '''), 4)

    async def test_closure(self, client):
        with self.assertRaisesRegex(
                BadDataError,
                r'closures cannot be used recursively'):
            await client.query(r'''
                .a = ||.map((b = .a));
                .map(.a);
            ''')

        with self.assertRaisesRegex(
                BadDataError,
                r'stored closures with side effects must be '
                r'wrapped using '):
            await client.query(r'''
                .b = ||(.x = 1);
                [1 ,2 ,3].map(.b);
            ''')

        with self.assertRaisesRegex(
                BadDataError,
                r'stored closures with side effects must be '
                r'wrapped using '):
            await client.query(r'''
                .a = [||.x = 1];
                [1 ,2 ,3].map(.a[0]);
            ''')

        # test two-level deep nesting
        self.assertEqual(await client.query(r'''
            .b = |k1|.map(|k2|(k1 + k2));
            .map(.b);
        '''), [["aa", "ab"], ["ba", "bb"]])

    async def test_set(self, client):
        self.assertTrue(await client.query(r'''
            ( set() == set() )
        '''))

        self.assertTrue(await client.query(r'''
            .a = {}; .b = .a;
            ( set(.a, .a) == set(.b,) )
        '''))

        self.assertTrue(await client.query(r'''
            .a = {};
            ( set([.a]) != set([]) )
        '''))

        self.assertTrue(await client.query(r'''
            .a = {}; .b = {};
            ( set([.a]) != set([.b]) )
        '''))


if __name__ == '__main__':
    run_test(TestTypes())

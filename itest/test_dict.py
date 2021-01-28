#!/usr/bin/env python
import asyncio
import pickle
import time
import random
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


class TestDict(TestBase):

    title = 'Test thing as dictionary '

    @default_test_setup(num_nodes=2, seed=1, threshold_full_storage=10)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        # add another node otherwise backups are not possible
        # await self.node1.join_until_ready(client)

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_functions(self, client):
        keys = [
            "",
            "a",
            "b",
            "this is a key",
        ]
        res = await client.query(r'''
            .dict = {};
            .dict["this is a key"] = 42;
            .dict.set("", "empty key");
            .dict.set("a", 1);
            .dict.set("b", 2);
            .dict.filter(||true);
        ''')
        self.assertEqual(res, {
            "this is a key": 42,
            "": "empty key",
            "a": 1,
            "b": 2,
        })
        res = await client.query(r'''
            a = {};
            a[""] = 1;
            a[""] += 1;
            a[""] = a.get("") + 1;
            a;
        ''')
        self.assertEqual(res, {"": 3})
        res = await client.query(r'''
            .dict.del("");
        ''')
        self.assertEqual(res, "empty key")
        res = await client.query(r'''
            x = {}; x[""] = "!empty";
            .dict.assign(x);
            nil;
        ''')
        self.assertEqual(res, None)
        res = await client.query(r'''.dict.len();''')
        self.assertEqual(res, 4)
        res = await client.query(r'''.dict.keys()''')
        self.assertEqual(res, keys)
        res = await client.query(r'''
            keys = [];
            .dict.each(|k| keys.push(k));
            keys;
        ''')
        self.assertEqual(res, keys)
        res = await client.query(r'''.dict.map(|k| k);''')
        self.assertEqual(res, keys)

        res = await client.query(r'''.dict.values();''')
        self.assertEqual(res, [
            "!empty",
            1,
            2,
            42
        ])

        # object <- dict
        res = await client.query(r'''
            x = {
                other: 123,
            };
            x.assign(.dict);
            x;
        ''')
        self.assertEqual(res, {
            "this is a key": 42,
            "": "!empty",
            "a": 1,
            "b": 2,
            "other": 123
        })
        # dict <- dict
        res = await client.query(r'''
            x = {};
            x["!x"] = 666;
            x.assign(.dict);
            x;
        ''')
        self.assertEqual(res, {
            "this is a key": 42,
            "": "!empty",
            "a": 1,
            "b": 2,
            "!x": 666
        })
        # type <- dict
        res = await client.query(r'''
            set_type('A', {name: 'str'});
            a = A{name: 'Iris'};
            b = {name: 'Cato'};
            b[""] = 123;
            b.del("");
            a.assign(b);
            a;
        ''')
        self.assertEqual(res, {
            "name": "Cato"
        })
        # dict <- type
        res = await client.query(r'''
            .dict.assign(A{});
            .dict.filter(|_, v| is_str(v));
        ''')
        self.assertEqual(res, {
            "": "!empty",
            "name": ""
        })
        with self.assertRaisesRegex(
                LookupError,
                r'type `A` has no property '
                r'`#this is a very simple test with a very long...`'):
            await client.query(r'''
                x = {};
                x["#this is a very simple test with a very long key"] = 123;
                A{}.assign(x);
            ''')

        res = await client.query(r'''
            dt = datetime('2021-01-28');
            x = {};
            x[""] = nil;
            x["day"] = 6;
            x["month"] = 2;
            dt.replace(x);
        ''')
        self.assertEqual(res, '2021-02-06T00:00:00Z')
        res = await client.query(r'''
            [
                .dict.has(""),
                .dict.has("a"),
                .dict.has("this is a key"),
                .dict.has("a b c"),
                .dict.has("z"),
                {}.has("a"),
                {}.has("")
            ];
        ''')
        self.assertEqual(res, [True, True, True, False, False, False, False])

    async def test_errors(self, client):
        with self.assertRaisesRegex(
                ValueError,
                'property `#` is reserved'):
            await client.query(r'x = {}; x["#"] = 123;')

        with self.assertRaisesRegex(
                ValueError,
                'property `#` is reserved'):
            await client.query(r'x = {}; x.set("#", 123);')

        with self.assertRaisesRegex(
                ValueError,
                'property `#` is reserved'):
            await client.query(r'', x={"#": 123})

    async def test_assign_and_del(self, client):
        await client.query(r'''
            .dict = {};
        ''')
        keys = []
        for i in range(0, 2000, 50):
            key = [chr(random.randint(ord('0'), ord('z'))) for x in range(i)]
            keys.append(''.join(key))
        for key in keys:
            await client.query(r'''
                .dict.set(key, nil);
            ''', key=key)
        self.assertEqual(await client.query('.dict.len();'), len(keys))
        for key in keys:
            await client.query(r'''
                .dict.del(key);
            ''', key=key)
        self.assertEqual(await client.query('.dict.len();'), 0)

    async def test_dict_enum(self, client):
        with self.assertRaisesRegex(
                ValueError,
                'enum member must follow the naming rules;'):
            res = await client.query(r'''
                x = {}; x[""] = nil;
                x["yes"] = 0;
                x["no"] = 1;
                set_enum('C', x);
            ''')
        res = await client.query(r'''
            x = {}; x[""] = nil;
            x["yes"] = 0;
            x["no"] = 1;
            x.del("");
            set_enum('C', x);
            [C{yes}, C{no}];
        ''')
        self.assertEqual(res, [0, 1])

    async def test_dict_type(self, client):
        with self.assertRaisesRegex(
                ValueError,
                'xxx'):
            res = await client.query(r'''
                x = {}; x[""] = nil;
                x["name"] = 'str';
                x["age"] = 'int';
                set_type('T', x);
            ''')
        res = await client.query(r'''
            x = {}; x[""] = nil;
            x["name"] = 'str;
            x["age"] = 'int';
            x.del("");
            set_type('T', x);
            T{};
        ''')
        self.assertEqual(res, {name: '', age: 0})


if __name__ == '__main__':
    run_test(TestDict())

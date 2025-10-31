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


class TestWrapTree(TestBase):

    title = 'Test wrap with defined tree structure'

    @default_test_setup(num_nodes=1, seed=1, threshold_full_storage=10)
    async def async_run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_define_err(self, client):

        with self.assertRaisesRegex(
                ValueError,
                r'defining nested types is only allowed for wrap-only type; '
                r'\(happened on type `W`\)'):
            await client.query("""//ti
                set_type('W', {nested: {x: '#'}});
            """)

        with self.assertRaisesRegex(
                ValueError,
                r'type keys must follow the naming rules;'):
            await client.query("""//ti
                nested = {};
                nested['a b'] = '#';
                set_type('W', {nested:,}, true);
            """)

        with self.assertRaisesRegex(
                TypeError,
                r'type `W` has wrap-only mode enabled'):
            await client.query("""//ti
                set_type('W', {nested: {x: W{}}}, true);
            """)

        with self.assertRaisesRegex(
                ValueError,
                r'array with nested type must have exactly one '
                r'element but found 0 on type `W`'):
            await client.query("""//ti
                set_type('W', {nested: []}, true);
            """)

        with self.assertRaisesRegex(
                TypeError,
                r'element in array must be of type `thing` but got '
                r'type `int` instead; '
                r'\(happened on type `W`\)'):
            await client.query("""//ti
                set_type('W', {nested: [123]}, true);
            """)

        with self.assertRaisesRegex(
                TypeError,
                r'invalid declaration for `a` on type `W`; unknown '
                r'type `UNKNOWN` in declaration;'):
            await client.query("""//ti
                set_type('W', {nested: [{a: 'UNKNOWN'}]}, true);
            """)

        with self.assertRaisesRegex(
                TypeError,
                r'expecting a nested structure \(`list` or `thing`\), '
                r'a method of type `closure` or a definition of type `str` '
                r'but got type `nil` instead;'):
            await client.query("""//ti
                set_type('W', {nested: nil}, true);
            """)

        with self.assertRaisesRegex(
                ValueError,
                r'nested type definition on type `W` too large'):
            await client.query("""//ti
                nested = {};
                range(2000).map(|x| {nested[`key{x}`] = {x: 'int'}});
                set_type('W', {nested:,}, true);
            """)

        with self.assertRaisesRegex(
                OperationError,
                r'type `W` contains a nested structure on field `nested` '
                r'which requires `wrap-only` mode to be enabled'):
            await client.query("""//ti
                set_type('W', {nested: {}}, true);
                mod_type('W', 'wpo', false);
            """)

        with self.assertRaisesRegex(
                TypeError,
                r'cannot convert a property into a method; '):
            await client.query("""//ti
                mod_type('W', 'mod', 'nested', ||nil);
            """)

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `mod_type` with task `mod` takes at '
                r'most 4 arguments when used on a type with wrap-only '
                r'mode enabled;'):
            await client.query("""//ti
                mod_type('W', 'mod', 'nested', 'str', ||nil);
            """)

    async def test_define(self, client):
        await client.query("""//ti
            set_type('W', {
                nested: {
                    x: '#'
                },
                other: {
                    y: 'int'
                },
                arr: [{
                    o: {
                        id: '#',
                        key: |this| `key{this.id()}`,
                    }
                }]
            }, true);
        """)

        res = await client.query("""//ti
            .x = {nested: {}, other: {}, arr: [{o: {}}]};
            .x.wrap('W');
        """)
        self.assertTrue(isinstance(res['nested']['x'], int))
        self.assertEqual(len(res['other']), 0)
        self.assertEqual(len(res['arr']), 1)
        self.assertEqual(len(res['arr'][0]), 1)
        self.assertEqual(len(res['arr'][0]['o']), 2)
        self.assertEqual(
            f"key{res['arr'][0]['o']['id']}",
            res['arr'][0]['o']['key'])

        res = await client.query("""//ti
            type_info('W');
        """)
        self.assertEqual(sorted(res['fields']), sorted([
            ['nested', {'x': '#'}],
            ['other', {'y': 'int'}],
            ['arr', [{'o': {
                'id': '#',
                'key': '|this|`key{this.id()}`'
            }}]],
        ]))

    async def test_mod_type(self, client):
        await client.query("""//ti
            new_type('O');
            new_type('T');
            set_enum('E', {A: 'a'});
            set_type('T', {name: 'str', t: 'T?', o: 'O'});
            set_type('O', {arr: '[thing]'});
            set_type('W', {
                name: 'str',
                arr: [{
                    e: 'E',
                    w: 'W',
                    o: 'O',
                    t: 'T'
                }]
            }, true);
        """)

        await client.query("""//ti
            rename_type('O', 'OO');
            rename_type('T', 'TT');
            rename_type('W', 'WW');
            rename_enum('E', 'EE');
        """)

        res = await client.query("""//ti
            type_info('WW');
        """)
        self.assertEqual(res['fields'], [
            ['name', 'str'],
            ['arr', [{
                'e': 'EE',
                'o': 'OO',
                't': 'TT',
                'w': 'OO',
            }]]
        ])

        await client.query("""//ti
            mod_type('WW', 'ren', 'arr', 'obj');
            mod_type('WW', 'mod', 'obj', {
                e: 'EE',
                o: 'OO',
                t: 'TT',
                w: 'OO',
            });
        """)
        res = await client.query("""//ti
            type_info('WW');
        """)
        self.assertEqual(res['fields'], [
            ['name', 'str'],
            ['obj', {
                'e': 'EE',
                'o': 'OO',
                't': 'TT',
                'w': 'OO'
            }]
        ])

        await client.query("""//ti
            mod_type('WW', 'del', 'obj');
            mod_type('WW', 'add', 'o', {
                i: '#',
                e: {
                    v: 'EE?'
                }
            });
        """)

        res = await client.query("""//ti
            type_info('WW');
        """)

        self.assertEqual(res['fields'], [
            ['name', 'str'],
            ['o', {
                'i': '#',
                'e': {
                    'v': 'EE?'
                }
            }]
        ])

    async def test_pr_example(self, client):
        await client.query("""//ti
            set_type('M1', {
                id: '#',
                num: 'int',
            }, true);

            set_type('T1', {
                metrics: '&[M1]',
            }, true, true);

            set_type('C1', {
                x: '#',
                name: 'str',
                type: '&T1',
            }, true);

            set_type('C2', {
                x: '#',
                name: 'str',
                type: {
                    metrics: [{
                        id: '#',
                        num: 'int',
                    }],
                },
            }, true);
        """)

        C1, C2 = await client.query("""//ti
            .x = {
                name: 'example',
                type: {
                    metrics: set({
                           num: 1
                    }, {
                           num: 2
                    })
                }
            };
            [.x.wrap('C1'), .x.wrap('C2')];
        """)
        self.assertEqual(C1, C2)


if __name__ == '__main__':
    run_test(TestWrapTree())

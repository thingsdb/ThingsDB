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
from thingsdb.exceptions import NumArgumentsError
from thingsdb.exceptions import OperationError
from thingsdb.exceptions import OverflowError
from thingsdb.exceptions import SyntaxError
from thingsdb.exceptions import TypeError
from thingsdb.exceptions import ValueError
from thingsdb.exceptions import ZeroDivisionError


class TestAdvanced(TestBase):

    title = 'Test advanced, single node'

    @default_test_setup(num_nodes=1, seed=1, threshold_full_storage=10)
    async def run(self):

        self.node0.version()

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_qcache_recursion(self, client):
        with self.assertRaisesRegex(
                SyntaxError,
                'query syntax has reached the maximum recursion depth of 500'):
            await client.query(r"""//ti
                arr =
                    [[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[
                    [[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[
                    [[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[
                    [[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[
                    [[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[
                    [[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[
                    [[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[
                    [[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[
                    [[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[
                    [[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[
                    ]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]
                    ]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]
                    ]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]
                    ]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]
                    ]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]
                    ]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]
                    ]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]
                    ]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]
                    ]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]
                    ]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]];
            """)

        # The code below requires libcleri >=0.12.2, prior to this version,
        # libcleri woulld return with a recursion depth exception because the
        # depth was not decremented as should.
        res = await client.query(r"""//ti
            arr = [
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1];
        """)
        self.assertEqual(len(res), 532)

    async def test_extend_restrict(self, client):
        res = await client.query(r"""//ti
            set_type('A', {arr: '[str]'});
        """)
        with self.assertRaisesRegex(
                TypeError,
                r'type `int` is not allowed in restricted array'):
            await client.query(r"""//ti
                a = A{};
                a.arr.extend(range(3));
            """)

    async def test_closure_scope(self, client):
        res = await client.query(r'''
            a = 1;
            (|| a = 2).call();
            a;  // should be 1, not 2 as the closure should have it's own scope
        ''')
        self.assertEqual(res, 1)

        with self.assertRaisesRegex(
                LookupError,
                r'variable `a` is undefined'):
            await client.query(r'''
                (|| a = 2).call();
                a;  // should be undefined
            ''')

    async def test_block_cleanup(self, client):
        with self.assertRaisesRegex(
                LookupError,
                r'variable `x` is undefined'):
            await client.query(r'''
                try((|| {
                    x = {};
                    x.x = x;
                    1 / 0;
                }).call());
                x;  // should error, x is defined outside this scope
            ''')

    async def test_scope_new(self, client):
        res = await client.query(r'''
            a = 1;
            {
                a = 2;
            };
            a;
        ''')
        self.assertEqual(res, 2)
        res = await client.query(r'''
            a = 1;
            (|| {
                a = 2;
            }).call();
            a;
        ''')
        self.assertEqual(res, 1)

    async def test_combined(self, client):
        self.assertIs(await client.query(r'''
            range(1).each(|i|
                (i || a = 1) &&
                (a += i)
            );
        '''), None)

        with self.assertRaisesRegex(
                LookupError,
                r'variable `a` is undefined'):
            await client.query(r'''
                range(2).each(|i|
                    (i || a = 1) &&
                    (a += i)
                );
            ''')

    async def test_unpack(self, client):
        res = await client.query(r'''
            set_type('T', {
                name: 'str',
                age: 'int',
            });

            .t = T{name: 'test', age: 7};
        ''')

        with self.assertRaisesRegex(
                ValueError,
                r'property `#` is reserved;'):
            await client.query(r'''
                val;
            ''', val=res)

    async def test_set_assign(self, client):
        res = await client.query(r'''
            t = {
                s: set({}, {})
            };

            s = t.s;
            s;
        ''')
        self.assertEqual(res, [{}, {}])

    async def test_adv_rename(self, client):
        await client.query(r'''
            set_type('A', {name: 'str'});
            set_enum('C', {RED: 'F00'});
            set_type('Prev', {
                a: 'A',
                b: 'A?',
                c: '[A]',
                d: '[A?]',
                e: '[A?]?',
                f: '{A}',
                g: '{A}?',
                h: 'C',
                i: 'C?',
                j: '[C]',
                k: '[C?]',
                l: '[C?]?',
                m: 'Prev?',
            });
        ''')

        await client.query(r'''
            rename_type('A', 'Name');
            rename_type('Prev', 'New');
            rename_enum('C', 'Color');
        ''')

        res = await client.query(r'''
            type_info('New');
        ''')

        self.assertEqual(res['fields'], [
            ['a', 'Name'],
            ['b', 'Name?'],
            ['c', '[Name]'],
            ['d', '[Name?]'],
            ['e', '[Name?]?'],
            ['f', '{Name}'],
            ['g', '{Name}?'],
            ['h', 'Color'],
            ['i', 'Color?'],
            ['j', '[Color]'],
            ['k', '[Color?]'],
            ['l', '[Color?]?'],
            ['m', 'New?']])

    async def test_set_type_create(self, client):
        with self.assertRaisesRegex(
                OperationError,
                r'function `set_type` can only be used on a type without '
                r'active instances; 1 active instance '
                r'of type `AA` has been found;'):
            await client.query(r'''
                new_type('AA');

                set_type('AA', {
                    a: 'int',
                }, {
                    .aa = AA{};
                    false
                });
            ''')

        with self.assertRaisesRegex(
                OperationError,
                r'invalid declaration for `a` on type `B`; '
                r'cannot assign type `A` while the type is in use;'):
            await client.query(r'''
                set_type('A', {
                    x: 'int',
                }, {
                    set_type('B', {
                        a: 'A'
                    }, true);
                    true;
                });
            ''')

        res = await client.query(r'''
            .del('aa');
            set_type('AA', {
                a: 'int',
            }, (||{
                aa = AA{};
                false
            }).call());
            AA{};
        ''')
        self.assertEqual(res, {'a': 0})

    async def test_filter_things(self, client):
        res = await client.query(r'''
            set_type('X', {b: 'int'});
            a = [X{b:1}, X{b:2}, X{b:3}];
            a = a.filter( |x| x.b > 1 );
            type_count('X');
        ''')
        self.assertEqual(res, 2)

    async def test_wpo(self, client):
        await client.query(r'''
            set_type('Person', {
                name: 'str',
                age: 'int',
            }, false);
            set_type('_Name', {
                name: 'str'
            }, true);
        ''')

        with self.assertRaisesRegex(
                TypeError,
                r'invalid declaration for `wrap` on type `Foo`; '
                r'when depending on a type in wrap-only mode, both types '
                r'must have wrap-only mode enabled; either add a `\?` to '
                r'make the dependency nillable or make `Foo` a wrap-only '
                r'type as well;'):
            await client.query(r'''
                set_type('Foo', {
                    wrap: '_Name',
                });
            ''')

        await client.query(r'''
            set_type('_Foo1', {
                wrap: '_Name',
            }, true);
        ''')

        await client.query(r'''
            set_type('_Foo2', {
                wrap: '_Name?',
            }, false);
        ''')

        with self.assertRaisesRegex(
                OperationError,
                r'type `_Foo1` is dependent on at least one type '
                r'with `wrap-only` mode enabled'):
            await client.query(r'''
                mod_type('_Foo1', 'wpo', false);
            ''')

        with self.assertRaisesRegex(
                OperationError,
                r'type `_Foo2` is required by at least one other type '
                r'without having `wrap-only` mode enabled'):
            await client.query(r'''
                set_type('_Tmp2', {foo2: '_Foo2'});
                mod_type('_Foo2', 'wpo', true);
            ''')

        with self.assertRaisesRegex(
                TypeError,
                r'invalid declaration for `wrap` on type `_Foo2`; '
                r'when depending on a type in wrap-only mode, both types '
                r'must have wrap-only mode enabled; either add a `\?` to '
                r'make the dependency nillable or make `_Foo2` a wrap-only '
                r'type as well;'):
            await client.query(r'''
                mod_type('_Foo2', 'mod', 'wrap', '_Name', ||nil);
            ''')

        with self.assertRaisesRegex(
                TypeError,
                r'type `_Name` has wrap-only mode enabled'):
            await client.query(r''' n = _Name(); ''')

        with self.assertRaisesRegex(
                TypeError,
                r'type `_Name` has wrap-only mode enabled'):
            await client.query(r''' n = _Name{}; ''')

        with self.assertRaisesRegex(
                TypeError,
                r'type `_Name` has wrap-only mode enabled'):
            await client.query(r''' n = new('_Name'); ''')

        with self.assertRaisesRegex(
                OperationError,
                r'a type can only be changed to `wrap-only` mode without '
                r'having active instances; 1 active instance of '
                r'type `X` has been found;'):
            await client.query(r'''
                new_type('X');
                x = X{};
                mod_type('X', 'wpo', true);
            ''')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `mod_type` with task `add` takes at most 4 '
                r'arguments when used on a type with wrap-only mode enabled'):
            await client.query(r'''
                mod_type('_Foo1', 'add', 'x', 'int', ||1);
            ''')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `mod_type` with task `mod` takes at most 4 '
                r'arguments when used on a type with wrap-only mode enabled'):
            await client.query(r'''
                mod_type('_Foo1', 'mod', 'wrap', 'int', ||1);
            ''')

    async def test_conditions(self, client):
        with self.assertRaisesRegex(
                ValueError,
                r'invalid declaration for `a` on type `Foo`; '
                r'expecting a `<` character after `int`'):
            await client.query(r'''
                set_type('Foo', {a: 'int>'});
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid declaration for `a` on type `Foo`; '
                r'expecting a `<` character after `float`'):
            await client.query(r'''
                set_type('Foo', {a: 'float>'});
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid declaration for `a` on type `Foo`; '
                r'range <..> conditions expect a minimum and maximum value '
                r'and may only be applied to `int`, `float` or `str`'):
            await client.query(r'''
                set_type('Foo', {a: 'uint<0:10>'});
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid declaration for `a` on type `Foo`; '
                r'expecting a colon \(:\) after the minimum value '
                r'of the range;'):
            await client.query(r'''
                set_type('Foo', {a: 'float<>'});
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid declaration for `a` on type `Foo`; '
                r'the minimum value for a string range must not be negative'):
            await client.query(r'''
                set_type('Foo', {a: 'str<-1:5>'});
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid declaration for `a` on type `Foo`; '
                r'expecting the maximum value to be greater '
                r'than or equal to the minimum value;'):
            await client.query(r'''
                set_type('Foo', {a: 'int<-1: -5>'});
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid declaration for `a` on type `Foo`; '
                r'expecting a colon \(:\) after the minimum value '
                r'of the range'):
            await client.query(r'''
                set_type('Foo', {a: 'int<1>'});
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid declaration for `a` on type `Foo`; '
                r'expecting character `>` after `str<0:5`'):
            await client.query(r'''
                set_type('Foo', {a: 'str<0:5a>'});
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid declaration for `a` on type `Foo`; '
                r'expecting character `>` after `str<0:5`'):
            await client.query(r'''
                set_type('Foo', {a: 'str<0:5a>'});
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid declaration for `a` on type `Foo`; '
                r'not-a-number \(nan\) values are not allowed to '
                r'specify a range'):
            await client.query(r'''
                set_type('Foo', {a: 'float<nan:10>'});
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid declaration for `a` on type `Foo`; '
                r'expecting the maximum value to be greater than or '
                r'equal to the minimum value'):
            await client.query(r'''
                set_type('Foo', {a: 'float<inf:-inf>'});
            ''')

        with self.assertRaisesRegex(
                OverflowError,
                r'integer overflow'):
            await client.query(r'''
                set_type('Foo', {a: 'int<0:9223372036854775808>'});
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid declaration for `a` on type `Foo`; '
                r'the provided default value does not match the condition;'):
            await client.query(r'''
                set_type('Foo', {a: 'int<0:10:-5>'});
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid declaration for `a` on type `Foo`; '
                r'the provided default value does not match the condition;'):
            await client.query(r'''
                set_type('Foo', {a: 'str<1:10:>'});
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid declaration for `a` on type `Foo`; '
                r'the provided default value does not match the condition;'):
            await client.query(r'''
                set_type('Foo', {a: 'float<1.5:2.5:0>'});
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid declaration for `a` on type `Foo`; '
                r'a non-nillable pattern requires a valid default value;'):
            await client.query(r'''
                set_type('Foo', {a: '/abc/'});
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid declaration for `a` on type `Foo`; '
                r'a non-nillable pattern requires a valid default value;'):
            await client.query(r'''
                set_type('Foo', {a: '/abc/<def>'});
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r"cannot compile regular expression '/a\(bc/', "
                r"missing closing parenthesis"):
            await client.query(r'''
                set_type('Foo', {a: '/a(bc/'});
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid declaration for `a` on type `Foo`; '
                r'nested pattern conditions are not allowed;'):
            await client.query(r'''
                set_type('Foo', {a: '[/abc/]'});
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid declaration for `a` on type `Foo`; '
                r'pattern condition syntax is invalid;'):
            await client.query(r'''
                set_type('Foo', {a: '/abc'});
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid declaration for `a` on type `Foo`; '
                r'pattern condition syntax is invalid;'):
            await client.query(r'''
                set_type('Foo', {a: '/abc/<'});
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid declaration for `a` on type `Foo`; '
                r'pattern condition syntax is invalid;'):
            await client.query(r'''
                set_type('Foo', {a: '/abc/>'});
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid declaration for `a` on type `Foo`; '
                r'nested range conditions are not allowed;'):
            await client.query(r'''
                set_type('Foo', {a: '[int<0:10>]'});
            ''')

        with self.assertRaisesRegex(
                TypeError,
                r'invalid declaration for `a` on type `Foo`; '
                r'unknown type ` ` in declaration'):
            await client.query(r'''
                set_type('Foo', {a: '[ ]'});
            ''')

        res = await client.query(r'''
            set_type('Foo', {
                int_a: 'int<0:10>',
                int_b: 'int<1:10>',
                int_c: 'int<-5:-3>',
                int_d: 'int<-5:5>',
                int_e: 'int<0:10:5>',
                int_f: 'int<0:10:5>?',
                int_g: 'int<0:10>?',
                float_a: 'float<-1:1>',
                float_b: 'float<0.5:1.5>',
                float_c: 'float<0:1:0.5>',
                float_d: 'float<0:inf>',
                float_e: 'float<-5:5:0>?',
                float_f: 'float<-5:5>?',
                str_a: 'str<0:1>',
                str_b: 'str<1:1>',
                str_c: 'str<3:10>',
                str_d: 'str<0:10:unknown>',
                str_e: '/^(e|h|i|l|o)*$/',
                str_f: '/^(e|h|i|l|o)+$/?',
                str_g: '/^(e|h|i|l|o)+$/i?',
                str_h: '/^(e|h|i|l|o)+$/i<Hello>',
                str_i: '/^(e|h|i|l|o){2}$/i<Hi>',
                str_j: 'str<5:10:empty>?',
                str_k: 'str<5:5>?',
            });

            Foo();
        ''')

        self.assertEqual(res, {
            "int_a": 0,
            "int_b": 1,
            "int_c": -3,
            "int_d": 0,
            "int_e": 5,
            "int_f": 5,
            "int_g": None,
            "float_a": 0.0,
            "float_b": 0.5,
            "float_c": 0.5,
            "float_d": 0,
            "float_e": 0,
            "float_f": None,
            "str_a": "",
            "str_b": "-",
            "str_c": "---",
            "str_d": "unknown",
            "str_e": "",
            "str_f": None,
            "str_g": None,
            "str_h": "Hello",
            "str_i": "Hi",
            "str_j": "empty",
            "str_k": None,
        })

        self.assertEqual(await client.query(r'''
                Foo{int_a: 0};
                Foo{int_a: 10};
                Foo{int_d: -5};
                Foo{int_d: 5};
                Foo{int_f: 5};
                Foo{int_f: nil};
                Foo{float_d: 3.14};
                Foo{float_d: inf };
                Foo{float_d: 42.0};
                Foo{float_e: -2.5};
                Foo{float_e: nil};
                Foo{str_a: ""};
                Foo{str_a: "a"};
                Foo{str_b: "b"};
                Foo{str_c: "abc"};
                Foo{str_c: "abcdefghij"};
                Foo{str_e: ""};
                Foo{str_e: "hello"};
                Foo{str_e: "hi"};
                Foo{str_f: nil};
                Foo{str_g: "HeLLo"};
                Foo{str_i: "He"};
                Foo{str_k: nil};
                Foo{str_k: "found"};
                42;  // reached the end
        '''), 42)

        with self.assertRaisesRegex(
                ValueError,
                r'mismatch in type `Foo`; '
                r'property `int_a` requires a float value between '
                r'0 and 10 \(both inclusive\)'):
            await client.query(r'''
                Foo{int_a: -1};
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'mismatch in type `Foo`; '
                r'property `int_a` requires a float value between '
                r'0 and 10 \(both inclusive\)'):
            await client.query(r'''
                Foo{int_a: 11};
            ''')

        with self.assertRaisesRegex(
                TypeError,
                r'mismatch in type `Foo`; '
                r'type `float` is invalid for property `int_a` with '
                r'definition `int<0:10>'):
            await client.query(r'''
                Foo{int_a: 5.5};
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'mismatch in type `Foo`; '
                r'property `float_d` requires a float value between '
                r'0.000000 and inf \(both inclusive\)'):
            await client.query(r'''
                Foo{float_d: -1.0};
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'mismatch in type `Foo`; '
                r'property `float_d` requires a float value between '
                r'0.000000 and inf \(both inclusive\)'):
            await client.query(r'''
                Foo{float_d: -inf };
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'mismatch in type `Foo`; '
                r'property `float_d` requires a float value between '
                r'0.000000 and inf but got nan \(not a number\)'):
            await client.query(r'''
                Foo{float_d: -nan };
            ''')

        with self.assertRaisesRegex(
                TypeError,
                r'mismatch in type `Foo`; '
                r'type `str` is invalid for property `float_d` with '
                r'definition `float<0:inf>'):
            await client.query(r'''
                Foo{float_d: 'pi'};
            ''')

        with self.assertRaisesRegex(
                TypeError,
                r'mismatch in type `Foo`; '
                r'type `nil` is invalid for property `float_d` with '
                r'definition `float<0:inf>'):
            await client.query(r'''
                Foo{float_d: nil};
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'mismatch in type `Foo`; '
                r'property `str_b` requires a string with a length '
                r'of 1 character'):
            await client.query(r'''
                Foo{str_b: ""};
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'mismatch in type `Foo`; '
                r'property `str_c` requires a string with a length '
                r'between 3 and 10 \(both inclusive\) characters'):
            await client.query(r'''
                Foo{str_c: "xx"};
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'mismatch in type `Foo`; '
                r'property `str_k` requires a string with a length '
                r'of 5 characters'):
            await client.query(r'''
                Foo{str_k: "ABCDEF"};
            ''')

        with self.assertRaisesRegex(
                TypeError,
                r'type `nil` is invalid for property `str_e` with '
                r'definition `/\(e|h|i|l|o\)\*/`'):
            await client.query(r'''
                Foo{str_e: nil};
            ''')
        with self.assertRaisesRegex(
                ValueError,
                r'property `str_f` has a requirement to match '
                r'pattern /\(e|h|i|l|o\)\+/"'):
            await client.query(r'''
                Foo{str_f: ""};
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'mismatch in type `Foo`; '
                r'property `str_f` has a requirement to match '
                r'pattern /^\(e|h|i|l|o\)\+$'):
            await client.query(r'''
                Foo{str_f: "HDello"};
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'mismatch in type `Foo`; '
                r'property `str_i` has a requirement to match '
                r'pattern /^\(e|h|i|l|o\){2}$/i'):
            await client.query(r'''
                Foo{str_i: ""};
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'mismatch in type `Foo`; '
                r'property `str_i` has a requirement to match '
                r'pattern /^\(e|h|i|l|o\){2}$/i'):
            await client.query(r'''
                Foo{str_i: "Hello"};
            ''')

        res = await client.query(r'''
            set_type('Int_a1', { int_a: 'int' });
            set_type('Int_a2', { int_a: 'int<0:10>' });
            set_type('Int_a3', { int_a: 'int<-1:11>' });
            set_type('Int_a4', { int_a: 'int<0:9>' });
            set_type('Int_a5', { int_a: 'int<1:10>' });
            set_type('Int_a6', { int_a: 'float<0:10>' });
            f = Foo{};
            [
                f.wrap('Int_a1'),
                f.wrap('Int_a2'),
                f.wrap('Int_a3'),
                f.wrap('Int_a4'),
                f.wrap('Int_a5'),
                f.wrap('Int_a6'),
            ]
        ''')

        self.assertEqual(res, [
            {"int_a": 0},
            {"int_a": 0},
            {"int_a": 0},
            {},
            {},
            {},
        ])

        res = await client.query(r'''
            set_type('Float_a1', { float_a: 'float' });
            set_type('Float_a2', { float_a: 'float<-1:1>' });
            set_type('Float_a3', { float_a: 'float<-2:2>' });
            set_type('Float_a4', { float_a: 'float<-1:0.5>' });
            set_type('Float_a5', { float_a: 'float<-0.5:1>' });
            set_type('Float_a6', { float_a: 'int<0:10>' });
            f = Foo{};
            [
                f.wrap('Float_a1'),
                f.wrap('Float_a2'),
                f.wrap('Float_a3'),
                f.wrap('Float_a4'),
                f.wrap('Float_a5'),
                f.wrap('Float_a6'),
            ]
        ''')

        self.assertEqual(res, [
            {"float_a": 0.0},
            {"float_a": 0.0},
            {"float_a": 0.0},
            {},
            {},
            {},
        ])

        res = await client.query(r'''
            set_type('Str_c1', { str_c: 'str' });
            set_type('Str_c2', { str_c: 'str<3:10>' });
            set_type('Str_c3', { str_c: 'str<0:10>' });
            set_type('Str_c4', { str_c: 'str<4:10>' });
            set_type('Str_c5', { str_c: 'str<3:9>' });
            set_type('Str_c6', { str_c: '/.*/' });
            set_type('Str_h1', { str_h: '/^(e|h|i|l|o)+$/i<Hi>' });
            set_type('Str_h2', { str_h: '/^(e|h|i|l|o)+$/i?' });
            set_type('Str_h3', { str_h: '/^(e|h|i|l|o)+$/<hello>' });
            set_type('Str_h4', { str_h: '/.*/' });
            f = Foo{};
            [
                f.wrap('Str_c1'),
                f.wrap('Str_c2'),
                f.wrap('Str_c3'),
                f.wrap('Str_c4'),
                f.wrap('Str_c5'),
                f.wrap('Str_c6'),
                f.wrap('Str_h1'),
                f.wrap('Str_h2'),
                f.wrap('Str_h3'),
                f.wrap('Str_h4'),
            ]
        ''')

        self.assertEqual(res, [
            {"str_c": "---"},
            {"str_c": "---"},
            {"str_c": "---"},
            {},
            {},
            {},
            {"str_h": "Hello"},
            {"str_h": "Hello"},
            {},
            {},
        ])

    async def test_any_set(self, client):
        self.assertEqual(await client.query(r'''
            set_type('Foo', {a: 'any'});
            f = Foo{a: set()};
            f.a.add({name: 'Iris'});
            42;  // reached the end
        '''), 42)

    async def test_adv_definition(self, client):
        with self.assertRaisesRegex(
                TypeError,
                r'invalid declaration for `s` on type `Foo`; '
                r'type `set` cannot contain a nillable type'):
            await client.query(r'''
                set_type('Foo', {s: '{thing?}'});
            ''')

        with self.assertRaisesRegex(
                TypeError,
                r'invalid declaration for `s` on type `Foo`; '
                r'type `set` cannot contain type `int'):
            await client.query(r'''
                set_type('Foo', {s: '{int}'});
            ''')

        with self.assertRaisesRegex(
                TypeError,
                r'invalid declaration for `s` on type `Foo`; '
                r'type `set` cannot contain a nillable type'):
            await client.query(r'''
                new_type('A');
                set_type('Foo', {s: '{A?}'});
            ''')

        self.assertEqual(await client.query(r'''
            set_type('Foo', {s: '{thing}'});
            f = Foo{};
            f.s.add(Foo{});
            42;
        '''), 42)

    async def test_query_gc(self, client):
        self.assertEqual(await client.query(r'''
            x = {};
            x.x = x;
            x = 5;
        '''), 5)

        self.assertEqual(await client.query(r'''
            x = {};
            x.y = {};
            x.y.y = x.y;
            x.set('y', 5);
        '''), 5)

        self.assertEqual(await client.query(r'''
            x = [{}];
            x[0].x = x[0];
            x.pop();
        '''), {"x": {}})

        self.assertEqual(await client.query(r'''
            {
                x = [{}];
                x[0].x = x[0];
                x.pop();
            };
        '''), {"x": {}})

        self.assertEqual(await client.query(r'''
            {
                x = [{}];
                x[0].x = x[0];
                x.pop();
            };
            5;
        '''), 5)

        self.assertEqual(await client.query(r'''
            {
                {
                    x = [{}];
                    x[0].x = x[0];
                    x.pop();
                };
                5;
            }
        '''), 5)

        self.assertIs(await client.query(r'''
            !!{
                x = [{}];
                bool(x[0].x = x[0]);
                x.pop()
            };
        '''), True)

        self.assertIs(await client.query(r'''
            {
                x = [{}];
                bool(x[0].x = x[0]);
                x.pop()
            } && true;
        '''), True)

        self.assertIs(await client.query(r'''
            {
                x = [{}];
                bool(x[0].x = x[0]);
                x.pop()
            }.id();
        '''), None)

        self.assertEqual(await client.query(r'''
            x = {
                y: {}
            };
            x.y.x = x;
            x.y.z = {};
            x.y.z.z = x.y.z;
            x.y.z.arr = [x, x.y, x.y.z];
            5;
        '''), 5)

        self.assertEqual(await client.query(r'''
            x = {};
            x.  y = {};
            x.y.y = x.y;
            {x.del('y')}.id();
            5;
        '''), 5)

    async def test_mod_type_mod_advanced2(self, client):
        with self.assertRaisesRegex(
                OperationError,
                r'field `chat` on type `Room` is modified but at least one '
                r'instance got an inappropriate value from the migration '
                r'callback; to be compliant, ThingsDB has used the default '
                r'value for this instance; callback response: mismatch in '
                r'type `Room`; type `int` is invalid for property `chat` with '
                r'definition `Room\?`'):
            await client.query(r'''
                set_type('Chat', {
                    messages: '[str]'
                });

                set_type('Room', {
                    name: 'str',
                    chat: 'str'
                });

                .room_a = Room{name: 'room A'};
                .room_b = Room{name: 'room B'};

                mod_type('Room', 'mod', 'chat', 'Room?', |room| {
                    // Here, we can create "Room" since chat allows "any"
                    // value at this point. However, new rooms must be modified
                    // accordinly afterwards
                    Room{name: `sub{room.name}`, chat: 123};
                });
            ''')

        msg = await client.query('.room_a.chat.name;')
        self.assertEqual(msg, 'subroom A')

        msg = await client.query('.room_b.chat.name;')
        self.assertEqual(msg, 'subroom B')

        self.assertIs(await client.query('.room_a.chat.chat;'), None)
        self.assertIs(await client.query('.room_b.chat.chat;'), None)

    async def test_type_count(self, client):
        res = await client.query(r'''
            new_type("X");
            set_type("X", {other: 'X?'});

            x = X{};
            a = [X{}];
            s = set(X{});
            y = X{};
            y.other = X{};
            t = {
                x: X{},
                a: [X{}]
            };

            .x = X{};
            .a = [X{}];
            .s = set(X{});
            .y = X{};
            .y.other = X{};

            type_count('X');
        ''')
        self.assertEqual(res, 12)

        res = await client.query(r'''
            new_type("Y");
            set_type("Y", {other: 'Y?'});

            y = Y{};
            y.other = y;

            r = {};
            r.r = r;
            x = {
                y: y,
                r: r
            };

            type_count('Y');
        ''')
        self.assertEqual(res, 1)

    async def test_mod_to_any(self, client):
        res = await client.query('''
            set_type('X', {
                arr: '[int]'
            });
            .x = X{};

            mod_type('X', 'mod', 'arr', 'any');

            .x.arr.push('Hello!');
            .x.arr.len();
        ''')
        self.assertEqual(res, 1)

    async def test_mod_del_in_use(self, client):
        with self.assertRaisesRegex(
                OperationError,
                r'cannot change type `X` while one of the '
                r'instances is in use'):
            res = await client.query('''
                set_type('X', {
                    a: 'int',
                    b: 'int',
                    c: 'int'
                });
                .x = X{};
                i = 0;
                .x.map(|k, v| {
                    if(i == 0, mod_type('X', 'del', 'c'));
                    i += 1;
                });
            ''')

    async def test_new(self, client):
        res = await client.query('''
            set_type('Count', {
                arr: '[int]'
            });

            x = {arr: [1, 2, 3]};

            c = new('Count', x);
            c.arr.push(4);
            x.arr;
        ''')
        self.assertEqual(res, [1, 2, 3])

    async def test_reuse_var(self, client):
        res = await client.query('''
            x = true;
            count = refs(true);
            x = false;
            assert (refs(true) < count);
        ''')
        self.assertIs(res, None)

    async def test_array_arg(self, client):
        res = await client.query('''
            new_procedure('add', |arr, v| arr.push(v));
            .arr = range(3);
            run('add', .arr, 3);
            .arr;
        ''')
        self.assertEqual(res, list(range(3)))

        res = await client.query('''
            .arr = range(5);
            (|arr, v| arr.push(v)).call(.arr, 5);
            .arr;
        ''')
        self.assertEqual(res, list(range(5)))

    async def test_type_mod(self, client):
        await client.query('''
            set_type("A", {b: 'str'});
            set_type("B", {a: 'A'});
            set_type("X", {arr: '[str]'});
            .x = X{arr: []};
        ''')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid declaration for `b` on type `A`; '
                r'missing `\?` after declaration `B`; '
                r'circular dependencies must be nillable at least '
                r'at one point in the chain'):
            await client.query('''
                mod_type('A', 'mod', 'b', 'B');
            ''')

        await client.query('''
            mod_type('A', 'mod', 'b', 'B?', ||nil);
        ''')

        with self.assertRaisesRegex(
                OperationError,
                r'type `B` is used by at least one other type'):
            await client.query('''
                del_type('B');
            ''')

        self.assertEqual(
            await client.query('''
                a = X{arr:[]};
                mod_type('X', 'mod', 'arr', '[str?]');
                a.arr.push(nil);
                a.arr;
            '''),
            [None])

        await client.query('''
            .x.arr.push(nil);
        ''')

    async def test_events(self, client):
        await self.assertChange(client, r'''
            .arr = range(3);
        ''')
        await self.assertChange(client, r'''
            thing(.id()).arr.push(3);
        ''')
        await self.assertChange(client, r'''
            .get('arr').push(4);
        ''')
        await self.assertChange(client, r'''
            thing(.id()).set('a', []);
        ''')

    async def test_no_events(self, client):
        await self.assertNoChange(client, r'''
            arr = range(3);
        ''')
        await self.assertNoChange(client, r'''
            range(3).push(4);
        ''')

    async def test_make(self, client):
        await client.query('''
            new_procedure('create_host', |
                    env_id, name, address, probes,
                    labels, description
                | {
                env = thing(env_id);
                host = {
                    parent: env,
                    name: name,
                    address: address,
                    probes: probes,
                    labels: set(labels.map(|l| thing(l))),
                    description: description,
                };
                env.hosts.add(host);
                labels.map(|l| thing(l)._hosts.add(host));
                host.id();
            });
            new_procedure('add_container', |env_id, name| {
                env = thing(env_id);
                container = {
                    parent: env,
                    name: name,
                    children: set(),
                    users: set(),
                    hosts: set(),
                    labels: set(),
                    conditions: set(),
                };
                env.children.add(container);
                container.id();
            });
            .root = {
                name: 'a',
                parent: nil,
                children: set(),
                users: set(),
                hosts: set(),
                labels: set(),
                conditions: set()
            };
        ''')
        root = await client.query('.root.id()')
        self.assertGreater(root, 0)
        self.assertIsInstance(root, int)
        self.assertNotEqual(root, None)

        a = await client.run('add_container', root, 'a')
        self.assertGreater(a, root)

        b = await client.run('add_container', a, 'b')
        self.assertGreater(b, a)

        host_a = await client.run(
            'create_host',
            root, 'name', '127.0.0.1', [], [], '')
        self.assertGreater(host_a, b)

        host_b = await client.run(
            'create_host',
            root, 'name', '127.0.0.1', [], [], '')
        self.assertGreater(host_b, host_a)

    async def test_issue_90(self, client):
        name = await client.query(r'''
            set_type('P', {name: 'str?'});
            .person = P{name: 'Iris'};

            mod_type('P', 'mod', 'name', 'str<3:25>?', |p| p.name);
            .person.name = 'Cato';
        ''')
        self.assertEqual(name, 'Cato')

        name = await client.query(r'''
            try(mod_type('P', 'mod', 'name', 'str<6:25>?', |p| p.name));
            .person.name;
        ''')
        self.assertIs(name, None)

    async def test_issue_91(self, client):
        res = await client.query(r'''
            set_type('P', {
                name: 'str'
            });

            set_type('A', {
                reports: '{P}'
            });

            .list = [
                A{reports: set()},
                A{reports: set(P{name: 'p1a'}, P{name: 'p1b'})},
                A{reports: set(
                    P{name: 'p2a'},
                    P{name: 'p2b'},
                    P{name: 'p2c'})},
            ];

            try(mod_type('A', 'mod', 'reports', '[]', |x| x.reports));

            .list[0].reports.push(P{name: 'p0a'});
            .list[1].reports.push(P{name: 'p1c'}, P{name: 'p1d'});

            try(mod_type(
                'A',
                'mod',
                'reports',
                '[]',
                |x| x.reports.map(|p|p)));

            .list.map(|a| {
                a.reports.map(|p| p.name);
            });
        ''')
        self.assertEqual(res, [["p0a"], ["p1c", "p1d"], []])
        self.assertEqual(await client.query('type_count("P");'), 3)

    async def test_ref_count_mod(self, client):
        res = await client.query(r'''
            set_type('P', {
                name: 'str'
            });

            set_type('A', {
                reports: '{P}'
            });

            .list = [
                A{reports: set()},
                A{reports: set(P{name: 'p1a'}, P{name: 'p1b'})},
                A{reports: set(
                    P{name: 'p2a'},
                    P{name: 'p2b'},
                    P{name: 'p2c'})},
            ];

            .store = .list[1].reports;

            try(mod_type('A', 'mod', 'reports', '[]', |x| x.reports));

            .list[1].reports.push(P{name: 'p0a'});
            .store.map(|p| p.name).sort();
        ''')
        self.assertEqual(res, ['p1a', 'p1b'])
        self.assertEqual(await client.query('type_count("P");'), 3)

    async def test_order_types(self, client):
        res = await client.query(r'''
            new_type('bike');
            new_type('mtb');
            new_type('banana');
            new_type('bal');
            new_type('atb');
            new_type('bacteria');
            types_info();
        ''')
        names = [t['name'] for t in res]
        self.assertEqual(
            names,
            ['atb', 'bacteria', 'bal', 'banana', 'bike', 'mtb']
        )

    async def test_order_procedures(self, client):
        res = await client.query(r'''
            new_procedure('bike', ||nil);
            new_procedure('mtb', ||nil);
            new_procedure('banana', ||nil);
            new_procedure('bal', ||nil);
            new_procedure('atb', ||nil);
            new_procedure('bacteria', ||nil);
            procedures_info();
        ''')
        names = [p['name'] for p in res]
        self.assertEqual(
            names,
            ['atb', 'bacteria', 'bal', 'banana', 'bike', 'mtb']
        )

    async def test_order_enums(self, client):
        res = await client.query(r'''
            set_enum('bike', {X:0});
            set_enum('mtb', {X:0});
            set_enum('banana', {X:0});
            set_enum('bal', {X:0});
            set_enum('atb', {X:0});
            set_enum('bacteria', {X:0});
            enums_info();
        ''')
        names = [e['name'] for e in res]
        self.assertEqual(
            names,
            ['atb', 'bacteria', 'bal', 'banana', 'bike', 'mtb']
        )

    async def test_copy_set_to_list(self, client):
        res = await client.query(r'''
            set_type('P', {name: 'str'});
            set_type('T', {
                s: '{P}',
                a: '[P]',
            });

            p = P{name: 'Iris'};
            t = T{s: set(p), a: [p]};

            list = list(t.s);
            list.push(123);  // list should not be restricted

            set = set(t.a);
            set.add({});  // set should nog be restricted

            [list.len(), set.len()];
        ''')
        self.assertEqual(res, [2, 2])

        res = await client.query(r'''
            list = [];
            set = set(P{}, P{});
            list.push(set);
            assert( is_tuple(list[0]) );
            list;

        ''')
        self.assertEqual(res, [[{"name": ""}, {"name": ""}]])

    async def test_thing_id_closure(self, client):
        res = await client.query(r'''
            .x = || {
                thing(3);
            };
            assert (is_closure(.x) );
        ''')
        self.assertIs(res, None)

    async def test_assign_in_def(self, client):
        res = await client.query(r'''
            str(|| x+=1);
        ''')
        self.assertEqual(res, '|| x += 1')

        res = await client.query(r'''
            str(|| .x += 1);
        ''')
        self.assertEqual(res, '|| .x += 1')

        res = await client.query(r'''
            || x[0]+=1;
        ''')
        self.assertEqual(res, '|| x[0]+=1')  # unbound closure

        res = await client.query(r'''
            .closure = || x[0]+=1;
            .closure;
        ''')
        self.assertEqual(res, '||x[0]+=1')  # bound closure

        res = await client.query(r'''
            str(.closure);
        ''')
        self.assertEqual(res, '|| x[0] += 1')  # formatted closure

    async def test_export(self, client):
        script = r'''
/*
 * Enums
 */

set_enum('B', {
    A: base64_decode('YUFh'),
    B: base64_decode('YkJi'),
    C: base64_decode('Y0Nj'),
});
set_enum('Colors', {
    RED: '#f00',
    GREEN: '#0f0',
    BLUE: '#00f',
});
set_enum('Math', {
    PI: 3.140000,
    E: 2.718000,
});
set_enum('Str', {
    sq: "Hi 'Iris'!",
    dq: 'Hi "Cato"!',
    bq: 'Hi ''"Tess"''!',
});

/*
 * Types
 */

new_type('Friend');
new_type('Person');

set_type('Friend', {
    person: 'Person',
    friend: 'Person',
});
set_type('Person', {
    name: 'str',
    age: 'int',
});


/*
 * Procedures
 */

new_procedure('more', |a, b| {
    .answers.push(a * b);
    .answers[-1];
});
new_procedure('multiply', |a, b| a * b);

'DONE';
'''.lstrip()
        await client.query(script)
        res = await client.query('export();')
        self.assertEqual(res, script)

    async def test_with_cache_one(self, client):
        await client.query(r'''
            set_type('Check', {
                name: 'str',
            });
            new_procedure('new_check', |name| {
                check = Check{
                    name: name,
                };
                .checks.push(check);
                check.id();
            });
            .checks = [];
            run('new_check', 'test');
        ''')

    async def test_with_cache_two(self, client):
        await client.query(r'''
            set_type('Check', {
                name: 'str',
            });
            new_procedure('new_check', |name| {
                check = Check{
                    name: name,
                };
                .checks.push(check);
                check.id();
            });
            .checks = [];
            run('new_check', 'test');
        ''')

    async def test_type_set(self, client):
        await client.query(r'''
            new_type('A');
            set_type('A', {
                aa: '{A}'
            });
        ''')

        await client.query(r'''
            a1 = A{};
            try(a1.aa |= set({}, {}));
            assert (a1.aa.len() == 0);
        ''')

    async def test_ren_after_mod(self, client):
        res = await client.query(r'''
            set_type('A', {m: ||nil});
            mod_type('A', 'mod', 'm', ||0);
            mod_type('A', 'ren', 'm', 'f');
            type_info('A');
        ''')
        methods = res['methods']
        self.assertIn('f', methods)
        self.assertNotIn('m', methods)
        self.assertEqual(len(methods), 1)

    async def test_mod_with_restriction(self, client):
        # bug #199
        res = await client.query(r"""//ti
            set_type('Test', {i: 'pint'});
            mod_type('Test', 'mod', 'i', 'int<0:200>', || 0);
            Test{};
        """)
        self.assertEqual(res, {"i": 0})

    async def test_mod_with_restriction(self, client):
        # bug #200
        with self.assertRaisesRegex(
                OperationError,
                r'cannot apply type declaration `int<0:200>` to `i` on '
                r'type `Test` without a closure to migrate existing '
                r'instances'):
            await client.query(r"""//ti
                set_type('Test', {i: 'pint'});
                mod_type('Test', 'mod', 'i', 'int<0:200>');
            """)

        res = await client.query(r"""//ti
            Test{};
        """)
        self.assertEqual(res, {"i": 1})
        # The above should not introduce a memory leak.

    async def test_fail_relations(self, client):
        # bug #203
        await client.query(r"""//ti
            new_type("A");
            new_type("B");
            set_type("A", {
                b: 'B?',
                bb: '{B}',
                c: 'B?',
                name: 'str'
            });
            set_type("B", {
                a: 'A?',
                aa: '{A}',
                cc: '{A}',
                name: 'str'
            });
            mod_type('A', 'rel', 'b', 'a');
            mod_type('A', 'rel', 'bb', 'aa');
            mod_type('A', 'rel', 'c', 'cc');
        """)

        res = await client.query(r"""//ti
            a = A{name: 'mr A'};
            try(b = B{
                a: a,
                name: 123
            });
            a.b;
        """)

        self.assertEqual(res, None)

        res = await client.query(r"""//ti
            a = A{name: 'mr A'};
            try(b = B{
                aa: set(a),
                name: 123
            });
            a.bb;
        """)
        self.assertEqual(res, [])

        res = await client.query(r"""//ti
            a = A{name: 'mr A'};
            try(b = B{
                cc: set(a),
                name: 123
            });
            a.c;
        """)
        self.assertEqual(res, None)

        res = await client.query(r"""//ti
            b = B{name: 'mr B'};
            try(a = A{
                c: b,
                name: 123
            });
            b.cc;
        """)
        self.assertEqual(res, [])

        res = await client.query(r"""//ti
            a = A{name: 'mr A'};
            x = new('B', {
                a: a,
                aa: set(a),
                cc: set(a),
                name: 'mr X'
            });
            try(b = new('B', {
                a: a,
                aa: set(a),
                cc: set(a),
                name: 123
            }));
            [a.b, a.bb.len(), a.c, x.a, x.aa.len(), x.cc];
        """)
        self.assertEqual(res, [None, 1, None, None, 1, []])

        res = await client.query(r"""//ti
            b = B{name: 'mr B'};
            x = A({
                b: b,
                bb: set(b),
                c: b,
                name: 'mr X'
            });
            try(a = A({
                b: b,
                bb: set(b),
                c: b,
                name: 123
            }));
            [b.a, b.aa.len(), b.cc.len(), x.b, x.bb.len(), is_thing(x.c)];
        """)
        self.assertEqual(res, [None, 1, 1, None, 1, True])

        res = await client.query(r"""//ti
            b = B{name: 'mr B'};
            x = A({
                b: b,
                bb: set(b),
                c: b,
                name: 'mr X'
            });
            try(x.assign({
                name: 'spy',
                b: nil
            }));
            x.name;
        """)
        self.assertEqual(res, 'mr X')

        res = await client.query(r"""//ti
            b = B{name: 'mr B'};
            x = A({
                b: b,
                bb: set(b),
                c: b,
                name: 'mr X'
            });
            x.assign(A{
                name: 'spy',
            });
            [x.name, x.b, x.bb, x.c, b.a];
        """)
        self.assertEqual(res, ['spy', None, [], None, None])

        res = await client.query(r"""//ti
            b = B{name: 'mr B'};
            x = A({
                b: b,
                bb: set(b),
                c: b,
                name: 'mr X'
            });
            d = B{name: 'md D'};
            c = A{
                name: 'spy',
                b: d,
                bb: set(d),
                c: d,
            };
            x.assign(c);
            [x.name, x.b == d, x.bb.len(), x.c == d, d.a == x, c.b];
        """)
        self.assertEqual(res, ['spy', True, 1, True, True, None])

    async def test_closure_as_type_val(self, client):
        # bug #202
        id = await client.query("""//ti
            set_type('Test', {
                func: 'any'
            });
            .test = Test{func: || .x = 1};
            .test.id();
        """)

        with self.assertRaisesRegex(
                OperationError,
                r"closures with side effects require a change but none is "
                r"created; use `wse\(...\)` to enforce a change;"):
            await client.query(f"""//ti
                thing({id}).func(); // requires a change
            """)

    async def test_future_to_type(self, client):
        await client.query(r"""//ti
            set_type('A', {x: 'any'});
        """)

        with self.assertRaisesRegex(
                TypeError,
                r'mismatch in type `A`; property `x` allows `any` type '
                r"with the exception of the `future` type"):
            await client.query("""//ti
                A{
                    x: future(||nil)
                };
            """)

    async def test_in_use_on_dict(self, client):
        # bug #242
        await client.query(".set('non name key', nil);")
        with self.assertRaisesRegex(
                OperationError,
                r'cannot change or remove property `arr` on `#\d+` while '
                r'the `list` is in use'):
            await client.query(r"""//ti
                .arr = ['a', 'b'];
                .arr.push({
                    .del('arr');
                    'c';
                })
            """)

    async def test_in_use_on_dict(self, client):
        # bug #243
        res = await client.query(r"""//ti
            new_type('A');
            t = [A{}.wrap()];
            .list = [];
            .list.push(t);
            // should return an Id
            .list[0][0].unwrap().id();
        """)
        self.assertIsInstance(res, int)

    async def test_index_name(self, client):
        # bug #252
        res = await client.query(r"""//ti
            {xxx: ''}.keys()[0][1];
        """)
        self.assertEqual(res, 'x')
        res = await client.query(r"""//ti
            {abcdefghij: ''}.keys()[0][2:8:2];
        """)
        self.assertEqual(res, 'ceg')

    async def test_slice_bytes(self, client):
        # feature #251
        res = await client.query(r"""//ti
            bytes('abc')[1];
        """)
        self.assertEqual(res, b'b')
        res = await client.query(r"""//ti
            bytes('abcdefghij')[2:8:2];
        """)
        self.assertEqual(res, b'ceg')

    async def test_replace_rev_large(self, client):
        # bug #253
        s = 'x'*17000
        res = await client.query(r"""//ti
            s.replace('z', ||nil, -1);
        """, s=s)
        self.assertEqual(s, res)

    async def test_loop_gc(self, client):
        res = await client.query(r"""//ti
            for (x in range(10)) {
                x = {};
                x.me = x;
            };
        """)
        # bug #259
        res = await client.query(r"""//ti
            range(10).sort(|a, b| {
                a = {};  // overwrite the closure argument
                a.me = a;  // create a self reference
                1;
            });
        """)


if __name__ == '__main__':
    run_test(TestAdvanced())

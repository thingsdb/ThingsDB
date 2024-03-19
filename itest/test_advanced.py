#!/usr/bin/env python
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client
from thingsdb.exceptions import AssertionError
from thingsdb.exceptions import LookupError
from thingsdb.exceptions import MaxQuotaError
from thingsdb.exceptions import NumArgumentsError
from thingsdb.exceptions import OperationError
from thingsdb.exceptions import OverflowError
from thingsdb.exceptions import SyntaxError
from thingsdb.exceptions import TypeError
from thingsdb.exceptions import ValueError


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
                r'additional info <..> can be applied to '
                r'`int`, `float`, `str`, `email`, `url` and `tel`;'):
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
                r'the minimum value for a string range must not be negative'):
            await client.query(r'''
                set_type('Foo', {a: 'utf8<-1:5>'});
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
                utf8_a: 'utf8<0:1>',
                utf8_b: 'utf8<1:1>',
                utf8_c: 'utf8<3:10>',
                utf8_d: 'utf8<0:10:unknown>',
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
            "utf8_a": "",
            "utf8_b": "-",
            "utf8_c": "---",
            "utf8_d": "unknown",
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
                r'property `int_a` requires an integer value between '
                r'0 and 10 \(both inclusive\)'):
            await client.query(r'''
                Foo{int_a: -1};
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'mismatch in type `Foo`; '
                r'property `int_a` requires an integer value between '
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
                r'of 1'):
            await client.query(r'''
                Foo{str_b: ""};
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'mismatch in type `Foo`; '
                r'property `str_c` requires a string with a length '
                r'between 3 and 10 \(both inclusive\)'):
            await client.query(r'''
                Foo{str_c: "xx"};
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'mismatch in type `Foo`; '
                r'property `str_k` requires a string with a length '
                r'of 5'):
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
            !!({
                x = [{}];
                bool(x[0].x = x[0]);
                x.pop()
            });
        '''), True)

        self.assertIs(await client.query(r'''
            {
                x = [{}];
                bool(x[0].x = x[0]);
                x.pop()
            } && true;
        '''), True)

        self.assertIs(await client.query(r'''
            ({
                x = [{}];
                bool(x[0].x = x[0]);
                x.pop()
            }).id();
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
            {x.del('y')};
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
                    if (i == 0) mod_type('X', 'del', 'c');
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
        self.assertEqual(res, list(range(4)))

        res = await client.query('''
            .arr = range(5);
            (|arr, v| arr.push(v)).call(.arr, 5);
            .arr;
        ''')
        self.assertEqual(res, list(range(6)))

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
// Enums

set_enum('B', {
  A: base64_decode('YUFh'),
  B: base64_decode('YkJi'),
  C: base64_decode('Y0Nj'),
});
set_enum('Colors', {
  RED: '#f00',
  GREEN: '#0f0',
  BLUE: '#00f',
  repr: |this| `{this.name()}={this.value()}`,
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

// Types

new_type('Friend');
new_type('Person');

set_type('Friend', {
  person: 'Person',
  friend: 'Person',
});
set_type('Person', {
  name: 'str',
  age: 'int',
  upper: |this| this.name..upper(),
});


// Procedures

new_procedure('more', |a, b| {
  .answers.push(a * b);
  .answers[-1];
});
new_procedure('multiply', |a, b| a * b);

'DONE';
'''.lstrip().replace('  ', '\t')
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
            .x = new('B');
            .x.a = a;
            .x.aa = set(a);
            .x.cc = set(a);
            .x.name = 'mr X';

            .b = new('B');
            .b.a = a;
            .b.aa = set(a);
            .b.cc = set(a);
            .b.a = nil;
            .b.aa = set();
            .b.cc = set();

            [a.b, a.bb.len(), a.c, .x.a, .x.aa.len(), .x.cc];
        """)
        self.assertEqual(res, [None, 1, None, None, 1, []])

        with self.assertRaisesRegex(
                TypeError,
                r"mismatch in type `A` on property `b`; "
                r"relations must be created using a property on a "
                r"stored thing \(a thing with an Id\)"):
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
                r"with the exception of the `future` and `module` type"):
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

    async def test_convert_to_thing(self, client):
        # bug #277
        res = await client.query(r"""//ti
            set_type('Root', {
                name: 'str'
            });
            .to_type('Root');
        """)
        res = await client.query(r"""//ti
            del_type('Root');
            .x = 123;  // should work as type Root is removed
                       // thus the collection must be a thing again.
            'OK';
        """)
        self.assertEqual(res, 'OK')

    async def test_self_ref_max(self, client):
        await client.query(r"""//ti
            set_type('A', {});
        """)
        for x in range(255):
            await client.query(r"""//ti
                mod_type('A', 'add', prop, 'A?');
            """, prop=f'p{x}')

        with self.assertRaisesRegex(
                MaxQuotaError,
                r'invalid declaration for `too_much` on type `A`; '
                r'maximum number of self references has been reached'):
            await client.query(r"""//ti
                mod_type('A', 'add', prop, 'A?');
            """, prop='too_much')

    async def test_ren_type_typed(self, client):
        # bug #292 (rename with a restricted type)
        await client.query(r"""//ti
            new_type('A');
            new_type('B');
            new_type('C');
            new_type('D');

            set_type('A', {
                a: 'A?',
                b: 'B',
                ta: 'thing<A>',
                tb: 'thing<B>?',
                tc: 'thing<C?>',
                td: 'thing<D?>?',
                la: '[A]',
                lb: '[B]?',
                lc: '[C?]',
                ld: '[D?]?',
                sa: '{A}',
                sb: '{B}?',
            });
        """)

        aa = await client.query(r"""//ti
            set_type('W', {
                name: 'any',
                fields: 'any'
            });
            rename_type('A', 'AA');
            rename_type('B', 'BB');
            rename_type('C', 'CC');
            rename_type('D', 'DD');
            type_info('AA').load().wrap('W');
        """)
        self.assertEqual(aa, {
            "fields": [
                [
                    "a",
                    "AA?"
                ],
                [
                    "b",
                    "BB"
                ],
                [
                    "ta",
                    "thing<AA>"
                ],
                [
                    "tb",
                    "thing<BB>?"
                ],
                [
                    "tc",
                    "thing<CC?>"
                ],
                [
                    "td",
                    "thing<DD?>?"
                ],
                [
                    "la",
                    "[AA]"
                ],
                [
                    "lb",
                    "[BB]?"
                ],
                [
                    "lc",
                    "[CC?]"
                ],
                [
                    "ld",
                    "[DD?]?"
                ],
                [
                    "sa",
                    "{AA}"
                ],
                [
                    "sb",
                    "{BB}?"
                ]
            ],
            "name": "AA"
        })

    async def test_reserved_enum_union(self, client):
        # bug #294
        with self.assertRaisesRegex(
                ValueError,
                'name `enum` is reserved'):
            await client.query('new_type("enum");')

        with self.assertRaisesRegex(
                ValueError,
                'name `union` is reserved'):
            await client.query('new_type("union");')

    async def test_loop_set_relation_error(self, client):
        # bug 302
        with self.assertRaises(AssertionError):
            await client.query("""//ti
                new_type('Workspace');
                new_type('Person');
                set_type('Workspace', {
                    people: '{Person}'
                });
                set_type('Person', {
                    workspace: 'Workspace?'
                });
                mod_type('Person', 'rel', 'workspace', 'people');
                .foo = Workspace{};
                .alice = Person{};
                .alice.workspace = .foo;
                .foo.people.some(|| assert(0));
            """)

    async def test_cope_on_wse_relation(self, client):
        await client.query("""//ti
            new_type('A');
            new_type('B');

            set_type('A', {
                b: '{B}'
            });

            set_type('B', {
                a: '{A}'
            });

            mod_type('A', 'rel', 'b', 'a');

            .a = A{};
            .b = B{};
            .b.a.add(.a);
            .clr = || {
                .a.b.clear();
            }
        """)

        res = await client.query("""//ti
            .b.a.each(|| wse(.clr()));
            .b.a.len();  // 0
        """)
        self.assertEqual(res, 0)

        await client.query('.b.a.add(.a);')

        with self.assertRaises(OperationError):
            # see pr #303
            res = await client.query("""//ti
                wse();
                .b.a.each(|| .clr());
            """)

    async def test_type_wrap_change(self, client):
        res = await client.query("""//ti
            set_type('A', {
                name: 'str',
                calc: |this| wse(.change())
            });
            .change = || {
                .a.name = 'mrX';
            };
            .a = A{name: 'msY'};
            return .a.wrap(), 1, NO_IDS;
        """)
        self.assertEqual(res, {
            "name": "msY",
            'calc': 'failed to compute property; method has side effects'
        })

        res = await client.query("""//ti
            set_type('B', {
                name: 'str',
                calc: |this| .change()
            });
            .change = || {
                .name = 'mrX';
            };
            .b = B{name: 'msY'};
            return .b.wrap(), 1, NO_IDS;
        """)
        self.assertEqual(res, {
            "name": "msY",
            'calc': (
                'closures with side effects require a change but '
                'none is created; use '
                '`wse(...)` to enforce a change; see '
                'https://docs.thingsdb.io/v1/collection-api/wse')
        })

    async def test_no_declaration_after_flags(self, client):
        with self.assertRaisesRegex(
                ValueError,
                r'invalid declaration for `x` on type `A`; '
                r'missing declaration after flags;'):
            # bug #306
            await client.query(r"""//ti
                set_type("A", {x: "&"});
            """)

    async def test_(self, client):
        # bug #309
        res = await client.query("""//ti
            new_type('A');
            new_type('C');
            set_type('A',{
                c: 'C?',
            });
            set_type('C', {
                a: '{A}',
                add: |this, options| {
                    options.a = set();
                    this.assign(options.filter(|o| this.has(o)));
                }
            });
            set_enum('E', {
                C: C{}
            });
            .add = |options| {
                kind = E{C}.value();
                kind.add(options);
            };
            .add({});
            "OK";
        """)

    async def test_filter_assign(self, client):
        res = await client.query("""//ti
            a = {
                arr: [1, 2, 3]
            };
            b = a.filter(||true);
            b.arr.push(4);
            a.arr;
        """)
        self.assertEqual(res, [1, 2, 3])

    async def test_fast_set_update(self, client):
        res = await client.query("""//ti
            .myarr = [
                {name: 'a'},
                {name: 'b'},
                {name: 'c'},
                {name: 'd'},
                {name: 'e'}];
            .myset = set(.myarr[:2]);

            s = .myset;
            s ^= set(.myarr);

            .myset.len();
        """)
        self.assertEqual(res, 3)

    async def test_list_copy_dup(self, client):
        # bug #314
        res = await client.query(r"""//ti
            la = [[[1, 2, 3]]];
            lb = la.copy(1);
            [type(lb[0]), type(lb[0][0])];
        """)
        self.assertEqual(res, ['tuple', 'tuple'])

        res = await client.query(r"""//ti
            la = [[1, 2, 3]];
            lb = la.dup(1);
            type(lb[0]);
        """)
        self.assertEqual(res, 'tuple')

        res = await client.query(r"""//ti
            la = [1, 2, 3, {x: 1}];
            lb = la.copy();
            lb.push(4);
            lb[-2].x = 2;
            la;
        """)
        self.assertEqual(res, [1, 2, 3, {'x': 2}])

        res = await client.query(r"""//ti
            la = [1, 2, 3, {x: 1}];
            lb = la.copy(1);
            lb.push(4);
            lb[-2].x = 2;
            la;
        """)
        self.assertEqual(res, [1, 2, 3, {'x': 1}])

    async def test_general_id_and_map_id(self, client):
        # feature request, issue #327
        res = await client.query(r"""//ti
            new_type('A');
            A{}.wrap().id();   // id() should work on a wrapped type
        """)
        self.assertEqual(res, None)

        wrap_id, thing_id, task_id, room_id = await client.query("""//ti
            .wa = A{}.wrap();
            .ti = {id: || 'test'};  // build-in id() wins from this id()
            .ta = task(datetime().move('seconds', 30), ||'bla');
            .ro = room();
            [.wa.id(), .ti.id(), .ta.id(), .ro.id()];
        """)

        res = await client.query(r"""//ti
            [.wa, .ti, .ta, .ro].map_id();  // map_id() more general
        """)
        self.assertEqual(res, [wrap_id, thing_id, task_id, room_id])

    async def test_wrap_enum_thing(self, client):
        # bug #329
        ida, idb = await client.query("""//ti
            set_type('_P', {
                id: '#',
                name: |this| this.name.upper()
            }, true);
            set_type('_W', {
                e: '&_P'
            }, true);
            set_enum('E', {
                a: {name: 'Alice'},
                b: {name: 'Bob'},
            });
            set_type('T', {
                e: 'E'
            });
            [E{a}.value().id(), E{b}.value().id()];
        """)

        res = await client.query("""//ti
            T{}.wrap();
        """)
        self.assertEqual(res, {"e": {"#": ida}})

        res = await client.query("""//ti
            T{}.wrap('_W');
        """)
        self.assertEqual(res, {"e": {"id": ida, "name": "ALICE"}})

    async def test_deep_computed_props(self, client):
        # bug #331
        res = await client.query("""//ti
            set_type('_C', {
                cc: 'int'
            }, true);
            set_type('_B', {
                bb: 'int',
                c: '&_C'
            }, true);
            set_type('_A', {
                aa: 'int',
                b: |this| this.b.wrap('_B')
            }, true);
            set_type('C', {
                cc: 'int<::3>'
            });
            set_type('B', {
                bb: 'int<::2>',
                c: 'C'
            });
            set_type('A', {
                aa: 'int<::1>',
                b: 'B'
            });
            a = A{};
            a.wrap('_A');
        """)
        self.assertEqual(res, {
            "aa": 1,
            "b": {
                "bb": 2,
                "c": {
                    "cc": 3
                }
            }
        })

    async def test_recustion_wrap_computed(self, client):
        # bug #332
        res = await client.query("""//ti
            set_type('T', {
                r: || T{}.wrap('T')
            });
            T{}.wrap();
        """)
        self.assertIn('r', res)

    async def test_procedure_with_future(self, client):
        # bug #334
        res = await client.query("""//ti
            new_procedure('test', |a| {
                T{a:};
                T{a:a};
            });
            procedure_info('test');
        """)
        self.assertEqual(res['definition'], '|a| {\n\tT{a: };\n\tT{a: a};\n}')

    async def test_multiple_methods(self, client):
        # bug #343
        res = await client.query("""//ti
            set_type('T', {
                a: |this| {this; 1/0;},
                b: |this| {this; 1/0;},
            });
            T{}.wrap();
        """)
        self.assertEqual(res, {
            "a": "division or modulo by zero",
            "b": "division or modulo by zero"
        })

    async def test_enum_membet_to_str(self, client):
        # bug #344
        t, tid = await client.query("""//ti
            set_enum('E', {
                x: {test: 123}
            });
            [str(E()), E().value().id()];  // fails
        """)
        self.assertEqual(t, f'thing:{tid}')

    async def test_array(self, client):
        res = await client.query("""//ti
            set_type('A', {
                j: '[task]',
                e: '[email]',
                u: '[url]',
                t: '[tel]'
            });
            a = A{};
            a.j.push(task());
            a.e.push('info@thingsdb.io');
            a.u.push('https://thingsdb.io');
            a.t.push('112');
            a;
        """)
        self.assertEqual(res, {
            "j": ['task:nil'],
            "e": ['info@thingsdb.io'],
            "u": ['https://thingsdb.io'],
            "t": ['112']
        })

        with self.assertRaisesRegex(
                TypeError,
                r'type `int` is not allowed in restricted array'):
            await client.query(r"""//ti
                A{}.e.push(123);
            """)

        with self.assertRaisesRegex(
                ValueError,
                r'array is restricted to email addresses'):
            await client.query(r"""//ti
                A{}.e.push('test');
            """)

        with self.assertRaisesRegex(
                ValueError,
                r"array is restricted to URL's"):
            await client.query(r"""//ti
                A{}.u.push('test');
            """)

        with self.assertRaisesRegex(
                ValueError,
                r'array is restricted to telephone numbers'):
            await client.query(r"""//ti
                A{}.t.push('test');
            """)

    async def test_enum_wrong_scope(self, client):
        # bug #350
        with self.assertRaisesRegex(
                LookupError,
                r'no enumerators exists in the `@thingsdb` scope; '
                r'you might want to query a `@collection` scope\?'):
            await client.query(r"""//ti
                A{TEST};
            """, scope='/thingsdb')

    async def test_deep_copy_and_dup(self, client):
        # pr #355
        res = await client.query(r"""//ti
            a = {};
            a.a = a;
            b = a.dup(10);
            b == b.a.a.a;
        """)
        self.assertIs(res, True)
        res = await client.query(r"""//ti
            a = {};
            a.a = a;
            b = a.copy(10);
            b == b.a.a.a;
        """)
        self.assertIs(res, True)
        res = await client.query(r"""//ti
            a = {};
            a['for item thing'] = 1;
            a.a = a;
            b = a.copy(10);
            b == b.a.a.a;
        """)
        self.assertIs(res, True)
        res = await client.query(r"""//ti
            a = {};
            a['for item thing'] = 1;
            a.a = a;
            b = a.dup(10);
            b == b.a.a.a;
        """)
        self.assertIs(res, True)
        res = await client.query(r"""//ti
            new_type('T');
            set_type('T', {
                a: 'T?'
            });
            a = T{};
            a.a = a;
            b = a.dup(10);
            b == b.a.a.a;
        """)
        self.assertIs(res, True)
        res = await client.query(r"""//ti
            a = T{};
            a.a = a;
            b = a.copy(10);
            b == b.a.a.a;
        """)
        self.assertIs(res, True)

    async def test_adv_rel(self, client):
        # bug found and fixed in pull request #357
        res = await client.query(r"""//ti
            new_type('R');
            set_type('R', {
                name: 'str',
                parent: '{R}',
                r: 'R?'
            });
            .r = R{name: 'master'};
            .r.r = R{name: 'slave'};
            .r.r.parent.add(.r);
            mod_type('R', 'rel', 'parent', 'r');
        """)
        self.assertIs(res, None)

    async def test_future_name(self, client):
        # bug solved in v1.5.0
        await client.query(r"""//ti
            new_procedure("add_user", || {
                return future(|| {
                    user = thing();
                    .users.add(user);
                    user;
                });
            });
            .users = set();
        """)

        with self.assertRaisesRegex(
                LookupError,
                r'type `future` has no function `id`'):
            await client.query(r"""//ti
                user = add_user(); user.id();
            """)

    async def test_utf8_range(self, client):
        await client.query(r"""//ti
            set_type('A', {
                u: 'utf8'
            });
            set_type('B', {
                u: 'utf8<1:>'
            });
            set_type('C', {
                u: 'utf8<:2>'
            });
            set_type('D', {
                u: 'utf8<1:3>'
            });
            set_type('E', {
                u: 'utf8<2:2>'
            });
        """)
        with self.assertRaisesRegex(
                ValueError,
                r'mismatch in type `B`; property `u` requires a '
                r'string with a length of at least 1'):
            await client.query(r"""//ti
                B{u: ""};
            """)
        with self.assertRaisesRegex(
                ValueError,
                r'mismatch in type `C`; property `u` requires a string '
                r'with a length between 0 and 2 \(both inclusive\)'):
            await client.query(r"""//ti
                C{u: "aaa"};
            """)
        with self.assertRaisesRegex(
                ValueError,
                r'mismatch in type `D`; property `u` requires a string '
                r'with a length between 1 and 3 \(both inclusive\)'):
            await client.query(r"""//ti
                D{u: ""};
            """)
        with self.assertRaisesRegex(
                ValueError,
                r'mismatch in type `E`; property `u` requires a '
                r'string with a length of 2'):
            await client.query(r"""//ti
                E{u: ""};
            """)
        with self.assertRaisesRegex(
                ValueError,
                r'mismatch in type `B`; property `u` only accepts '
                r'valid UTF8 data'):
            await client.query(r"""//ti
                B{u: "😁"[:3]};
            """)

        res = await client.query(r"""//ti
            a = A{u: "A"};
            a.wrap('B')
        """)
        self.assertEqual(res, {})

        res = await client.query(r"""//ti
            b = B{u: "B"};
            b.wrap('A')
        """)
        self.assertEqual(res, {"u": "B"})

        res = await client.query(r"""//ti
            b = B{u: "B"};
            b.wrap('D')
        """)
        self.assertEqual(res, {})

        res = await client.query(r"""//ti
            d = D{u: "D"};
            d.wrap('B')
        """)
        self.assertEqual(res, {"u": "D"})


if __name__ == '__main__':
    run_test(TestAdvanced())

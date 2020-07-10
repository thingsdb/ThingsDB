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


class TestAdvanced(TestBase):

    title = 'Test advanced, single node'

    @default_test_setup(num_nodes=1, seed=1, threshold_full_storage=10)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

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

    async def test_adv_specification(self, client):
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
                r'new instance was made with an inappropriate value which in '
                r'response is changed to default by ThingsDB; mismatch in '
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
                r'instances is being used'):
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
        await self.assertEvent(client, r'''
            .arr = range(3);
        ''')
        await self.assertEvent(client, r'''
            thing(.id()).arr.push(3);
        ''')
        await self.assertEvent(client, r'''
            .get('arr').push(4);
        ''')
        await self.assertEvent(client, r'''
            thing(.id()).set('a', []);
        ''')

    async def test_no_events(self, client):
        await self.assertNoEvent(client, r'''
            arr = range(3);
        ''')
        await self.assertNoEvent(client, r'''
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


if __name__ == '__main__':
    run_test(TestAdvanced())

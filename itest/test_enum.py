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


class TestEnum(TestBase):

    title = 'Test enumerators'

    @default_test_setup(num_nodes=1, seed=1, threshold_full_storage=10)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_set_enum(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `set_enum`'):
            await client.query('nil.set_enum();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `set_enum` takes 2 arguments but 0 were given'):
            await client.query('set_enum();')

        with self.assertRaisesRegex(
                TypeError,
                'function `set_enum` expects argument 1 to be of type `str` '
                'but got type `nil` instead'):
            await client.query(r'set_enum(nil, {X: 0});')

        with self.assertRaisesRegex(
                ValueError,
                'function `set_enum` expects '
                'argument 1 to be a valid enum name'):
            await client.query(r'''set_enum('=invalid', {X: 0});''')

        with self.assertRaisesRegex(
                TypeError,
                'function `set_enum` expects argument 2 to be of type `thing` '
                'but got type `nil` instead'):
            await client.query(r'''set_enum('Color', nil);''')

        await client.query(r'''
            set_type('Brick', {
                color: 'str'
            });
        ''')

        with self.assertRaisesRegex(
                LookupError,
                'type `Brick` already exists'):
            await client.query(r'''set_enum('Brick', {X: 1});''')

        with self.assertRaisesRegex(
                ValueError,
                'name `nil` is reserved'):
            await client.query(r'''set_enum('nil', {X: 1});''')

        with self.assertRaisesRegex(
                ValueError,
                'cannot create an empty enum type'):
            await client.query(r'''
                set_enum('Color', {});
            ''')

        self.assertIs(await client.query(r'''
            set_enum('Color', {
                RED: '#FF0000',
                GREEN: '#00FF00',
                BLUE: '#0000FF'
            });'''), None)

        with self.assertRaisesRegex(
                LookupError,
                'enum `Color` already exists'):
            await client.query(r'''set_enum('Color', {X: 1});''')

    async def test_enum(self, client):
        self.assertIs(await client.query(r'''
            set_enum('Color', {
                RED: '#FF0000',
                GREEN: '#00FF00',
                BLUE: '#0000FF'
            });'''), None)

        with self.assertRaisesRegex(
                LookupError,
                'type `int` has no function `enum`'):
            await client.query('(1).enum();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `enum` takes 2 arguments but 1 was given'):
            await client.query('enum("00FF00");')

        with self.assertRaisesRegex(
                TypeError,
                'function `enum` expects argument 1 to be of type `str` '
                'but got type `nil` instead'):
            await client.query(r'enum(nil, "#00FF00");')

        with self.assertRaisesRegex(
                LookupError,
                r'enum `X` not found'):
            await client.query(r'enum("X", "#00FF00");')

        with self.assertRaisesRegex(
                ValueError,
                r'enum name must follow the naming rules'):
            await client.query(r'enum("!", "#00FF00");')

        with self.assertRaisesRegex(
                TypeError,
                r'enum `Color` is expecting a value of type `str` '
                r'but got type `int` instead'):
            await client.query(r'enum("Color", 1);')

        with self.assertRaisesRegex(
                LookupError,
                r'enum `Color` has no member with value `RED'):
            await client.query(r'enum("Color", "RED");')

        self.assertEqual(
            await client.query('enum("Color", "#00FF00");'),
            "#00FF00")

    async def test_del_enum(self, client):
        self.assertIs(await client.query(r'''
            set_enum('Color', {
                RED: '#FF0000',
                GREEN: '#00FF00',
                BLUE: '#0000FF'
            });
            set_type('Brick', {
                color: 'Color'
            });
            '''), None)

        with self.assertRaisesRegex(
                LookupError,
                'type `int` has no function `del_enum`'):
            await client.query('(1).del_enum();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `del_enum` takes 1 argument but 0 were given'):
            await client.query('del_enum();')

        with self.assertRaisesRegex(
                TypeError,
                'function `del_enum` expects argument 1 to be of type `str` '
                'but got type `nil` instead'):
            await client.query(r'del_enum(nil);')

        with self.assertRaisesRegex(
                LookupError,
                r'enum `X` not found'):
            await client.query(r'del_enum("X");')

        with self.assertRaisesRegex(
                ValueError,
                r'enum name must follow the naming rules'):
            await client.query(r'del_enum("");')

        with self.assertRaisesRegex(
                OperationError,
                r'enum `Color` is used by at least one type'):
            await client.query(r'del_enum("Color");')

        await client.query(r'''
            del_type('Brick');
            .color = Color{GREEN};
        ''')

        with self.assertRaisesRegex(
                OperationError,
                r'enum member `Color{GREEN}` is still being used'):
            await client.query(r'del_enum("Color");')

        await client.query(r'''.del("color");''')

        self.assertIs(await client.query(r'del_enum("Color");'), None)

    async def test_mod_enum(self, client):
        self.assertIs(await client.query(r'''
            set_enum('Color', {
                RED: '#FF0000',
                GREEN: '#00FF00',
                BLUE: '#0000FF'
            });
            '''), None)

        with self.assertRaisesRegex(
                LookupError,
                'type `str` has no function `mod_enum`'):
            await client.query('"Color".mod_enum();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `mod_enum` requires at least 3 arguments '
                'but 0 were given'):
            await client.query('mod_enum();')

        with self.assertRaisesRegex(
                TypeError,
                'function `mod_enum` expects argument 1 to be of type `str` '
                'but got type `nil` instead'):
            await client.query(r'mod_enum(nil, nil, nil);')

        with self.assertRaisesRegex(
                LookupError,
                r'enum `X` not found'):
            await client.query(r'mod_enum("X", nil, nil);')

        with self.assertRaisesRegex(
                ValueError,
                r'enum name must follow the naming rules'):
            await client.query(r'mod_enum("", nil, nil);')

        with self.assertRaisesRegex(
                TypeError,
                r'function `mod_enum` expects argument 2 to be of '
                r'type `str` but got type `nil` instead'):
            await client.query(r'mod_enum("Color", nil, nil);')

        with self.assertRaisesRegex(
                ValueError,
                r'function `mod_enum` expects argument 2 to be `add`, `del` '
                r'or `mod` but got `x` instead'):
            await client.query(r'mod_enum("Color", "x", "x");')

        # Section ADD
        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `mod_enum` with task `add` takes 4 arguments '
                r'but 3 were given'):
            await client.query(r'mod_enum("Color", "add", "x");')

        with self.assertRaisesRegex(
                TypeError,
                r'enumerators must not contain members of mixed types; '
                r'got both type `str` and `nil` for enum `Color'):
            await client.query(r'mod_enum("Color", "add", "YELLOW", nil);')

        with self.assertRaisesRegex(
                ValueError,
                r'enum values must be unique; the given value is already '
                r'used by `Color{GREEN}`'):
            await client.query(r'''
                mod_enum("Color", "add", "YELLOW", "#00FF00");
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'member `GREEN` on `Color` already exists'):
            await client.query(r'''
                mod_enum("Color", "add", "GREEN", "#00FF00");
            ''')

        self.assertIs(await client.query(r'''
                mod_enum("Color", "add", "YELLOW", "#FFFF00");
            '''), None)

        await client.query('.color = Color{YELLOW};')

        # Section MOD
        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `mod_enum` with task `mod` takes 4 arguments '
                r'but 3 were given'):
            await client.query(r'mod_enum("Color", "mod", "x");')

        with self.assertRaisesRegex(
                LookupError,
                r'enum `Color` has no member `x'):
            await client.query(r'mod_enum("Color", "mod", "x", "#EEEE11");')

        with self.assertRaisesRegex(
                TypeError,
                r'enumerators must not contain members of mixed types; '
                r'got both type `str` and `int` for enum `Color'):
            await client.query(r'mod_enum("Color", "mod", "YELLOW", 1);')

        with self.assertRaisesRegex(
                ValueError,
                r'enum values must be unique; the given value is '
                r'already used by `Color{GREEN}`'):
            await client.query(r'''
                mod_enum("Color", "mod", "YELLOW", "#00FF00");
            ''')

        self.assertIs(await client.query(r'''
                mod_enum("Color", "mod", "YELLOW", "#FFFF00");
            '''), None)

        self.assertIs(await client.query(r'''
                mod_enum("Color", "mod", "YELLOW", "#EEEE11");
            '''), None)

        # Section DEL
        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `mod_enum` with task `del` takes 3 arguments '
                r'but 4 were given'):
            await client.query(r'mod_enum("Color", "del", "x", "y");')

        with self.assertRaisesRegex(
                ValueError,
                r'function `mod_enum` expects argument 3 to '
                r'follow the naming rules'):
            await client.query(r'mod_enum("Color", "del", "!");')

        with self.assertRaisesRegex(
                LookupError,
                r'enum `Color` has no member `x`'):
            await client.query(r'mod_enum("Color", "del", "x");')

        with self.assertRaisesRegex(
                OperationError,
                r'enum member `Color{YELLOW}` is still being used'):
            await client.query(r'mod_enum("Color", "del", "YELLOW");')

        self.assertIs(await client.query(r'''
                .del("color");
                mod_enum("Color", "del", "YELLOW");
            '''), None)

    async def test_enum_info(self, client):
        self.assertIs(await client.query(r'''
            set_enum('Color', {
                RED: '#FF0000',
                GREEN: '#00FF00',
                BLUE: '#0000FF'
            });
            '''), None)

        with self.assertRaisesRegex(
                LookupError,
                'type `str` has no function `enum_info`'):
            await client.query('"Color".enum_info();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `enum_info` takes 1 argument '
                'but 0 were given'):
            await client.query('enum_info();')

        with self.assertRaisesRegex(
                TypeError,
                'function `enum_info` expects argument 1 to be of type `str` '
                'but got type `nil` instead'):
            await client.query(r'enum_info(nil);')

        with self.assertRaisesRegex(
                LookupError,
                r'enum `X` not found'):
            await client.query(r'enum_info("X");')

        with self.assertRaisesRegex(
                ValueError,
                r'enum name must follow the naming rules'):
            await client.query(r'enum_info("");')

        info = await client.query(r'enum_info("Color");')

        print(info)

        self.assertEqual(len(info), 5)
        self.assertTrue(isinstance(info['enum_id'], int))
        self.assertTrue(isinstance(info['name'], str))
        self.assertTrue(isinstance(info['created_at'], int))
        self.assertTrue(isinstance(info['members'], list))
        self.assertEqual(info['members'], [
            ['RED', '#FF0000'], ['GREEN', '#00FF00'], ['BLUE', '#0000FF']
        ])

    async def test_enums_info(self, client):
        self.assertIs(await client.query(r'''
            set_enum('Color', {
                RED: '#FF0000',
                GREEN: '#00FF00',
                BLUE: '#0000FF'
            });
            '''), None)

        with self.assertRaisesRegex(
                LookupError,
                'type `str` has no function `enums_info`'):
            await client.query('"Color".enums_info();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `enums_info` takes 0 arguments '
                'but 1 was given'):
            await client.query('enums_info(1);')

        enums_info = await client.query(r'enums_info();')

        info = enums_info[0]

        self.assertEqual(len(info), 5)
        self.assertTrue(isinstance(info['enum_id'], int))
        self.assertTrue(isinstance(info['name'], str))
        self.assertTrue(isinstance(info['created_at'], int))
        self.assertTrue(isinstance(info['members'], list))
        self.assertEqual(info['members'], [
            ['RED', '#FF0000'], ['GREEN', '#00FF00'], ['BLUE', '#0000FF']
        ])

    async def test_syntax(self, client):
        self.assertIs(await client.query(r'''
            set_enum('Color', {
                RED: '#FF0000',
                GREEN: '#00FF00',
                BLUE: '#0000FF'
            });
            '''), None)

    async def test_isenum(self, client):
        self.assertIs(await client.query(r'''
            set_enum('Color', {
                RED: '#FF0000',
                GREEN: '#00FF00',
                BLUE: '#0000FF'
            });
            '''), None)

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `isenum` takes 1 argument but 2 were given'):
            await client.query('isenum(1, 2);')

        self.assertTrue(await client.query(
            'isenum( enum("Color", "#FF0000") );'
        ))
        self.assertTrue(await client.query('isenum( Color{RED} ); '))
        self.assertTrue(await client.query('''
            isenum({
                color = "GREEN";
                Color{||color};
            });
        '''))

        self.assertFalse(await client.query('isenum( Color{RED}.value() ); '))
        self.assertFalse(await client.query('isenum( Color{RED}.name() ); '))
        self.assertFalse(await client.query('isenum( "RED" );'))
        self.assertFalse(await client.query('isenum( "#0000FF" );'))


if __name__ == '__main__':
    run_test(TestEnum())

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

        with self.assertRaisesRegex(
                TypeError,
                r'failed to create enum type `Color`; enumerators cannot '
                r'be created for values with type `bool`'):
            await client.query(r'''set_enum('Color', {X: true});''')

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

        with self.assertRaisesRegex(
                TypeError,
                r'invalid declaration for `colors` on type `FailSpec`; '
                r'type `set` cannot contain enum type `Color`'):
            await client.query(r'''
                set_type('FailSpec', {
                    colors: '{Color}'
                });
            ''')

        with self.assertRaisesRegex(
                OperationError,
                r'cannot change enum `Color` while the enumerator '
                r'is in use'):
            await client.query(r'''
                enum('Color', {
                    mod_enum('Color', 'add', 'YELLOW', '...');
                });
            ''')

        with self.assertRaisesRegex(
                OperationError,
                r'cannot change enum `Color` while the enumerator '
                r'is in use'):
            await client.query(r'''
                Color({
                    del_enum('Color');
                });
            ''')

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
                'function `enum` requires at least 1 argument '
                'but 0 were given'):
            await client.query('enum();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `enum` takes at most 2 arguments '
                'but 3 were given'):
            await client.query('enum("X", "#00FF00", nil);')

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
                r'enum member `Color{GREEN}` is still in use'):
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
                r'function `mod_enum` expects argument 2 to be `add`, `del`, '
                r'`mod` or `ren` but got `x` instead'):
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
                r'function `mod_enum` expects argument 3 to '
                r'follow the naming rules'):
            await client.query(r'''
                mod_enum("Color", "add", "#YELLOW", "#0000FF");
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'member `GREEN` on `Color` already exists'):
            await client.query(r'''
                mod_enum("Color", "add", "GREEN", "#00FF00");
            ''')

        with self.assertRaisesRegex(
                OperationError,
                r'cannot change enum `Color` while the enumerator '
                r'is in use'):
            await client.query(r'''
                mod_enum("Color", "add", "YELLOW", {
                    mod_enum("Color", "add", "YELLOW", "#FFFF00");
                    "#FFFF00";
                });
            ''')

        self.assertIs(await client.query(r'''
                mod_enum("Color", "add", "YELLOW", "#FFFF00");
            '''), None)

        await client.query('.color = Color{YELLOW};')

        # Section DEF
        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `mod_enum` with task `def` takes 3 arguments '
                r'but 4 were given'):
            await client.query(r'mod_enum("Color", "def", "x", "y");')

        with self.assertRaisesRegex(
                ValueError,
                r'function `mod_enum` expects argument 3 to '
                r'follow the naming rules'):
            await client.query(r'mod_enum("Color", "def", "!");')

        with self.assertRaisesRegex(
                LookupError,
                r'enum `Color` has no member `x`'):
            await client.query(r'mod_enum("Color", "def", "x");')

        self.assertIs(await client.query(r'''
                mod_enum("Color", "def", "BLUE");
            '''), None)
        self.assertEqual(await client.query(r'''
                enum('Color').name();
            '''), 'BLUE')

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

        # Section REN
        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `mod_enum` with task `ren` takes 4 arguments '
                r'but 3 were given'):
            await client.query(r'mod_enum("Color", "ren", "x");')

        with self.assertRaisesRegex(
                LookupError,
                r'enum `Color` has no member `x'):
            await client.query(r'mod_enum("Color", "ren", "x", "y");')

        with self.assertRaisesRegex(
                TypeError,
                r'function `mod_enum` with task `ren` expects argument 4 to '
                r'be of type `str` but got type `int` instead'):
            await client.query(r'mod_enum("Color", "ren", "YELLOW", 1);')

        with self.assertRaisesRegex(
                ValueError,
                r'member name must follow the naming rules'):
            await client.query(r'mod_enum("Color", "ren", "YELLOW", "#Y");')

        with self.assertRaisesRegex(
                ValueError,
                r'member `RED` on `Color` already exists'):
            await client.query(r'''
                mod_enum("Color", "ren", "YELLOW", "RED");
            ''')

        self.assertIs(await client.query(r'''
                mod_enum("Color", "ren", "YELLOW", "ORANGE");
            '''), None)

        self.assertIs(await client.query(r'''
                mod_enum("Color", "ren", "ORANGE", "ORANGE");
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
                r'enum member `Color{ORANGE}` is still in use'):
            await client.query(r'mod_enum("Color", "del", "ORANGE");')

        self.assertIs(await client.query(r'''
                .del("color");
                mod_enum("Color", "del", "ORANGE");
            '''), None)

        self.assertIs(await client.query(r'''
                mod_enum("Color", "del", "RED");
                mod_enum("Color", "del", "GREEN");
            '''), None)

        with self.assertRaisesRegex(
                LookupError,
                r'cannot delete `Color{BLUE}` as this is the last enum membe'):
            await client.query(r'mod_enum("Color", "del", "BLUE");')

    async def test_has_enum(self, client):
        self.assertIs(await client.query(r'''
            set_enum('Color', {
                RED: '#FF0000',
                GREEN: '#00FF00',
                BLUE: '#0000FF'
            });
            '''), None)

        with self.assertRaisesRegex(
                LookupError,
                'type `regex` has no function `has_enum`'):
            await client.query('/.*/.has_enum();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `has_enum` takes 1 argument but 0 were given'):
            await client.query('has_enum();')

        with self.assertRaisesRegex(
                TypeError,
                'function `has_enum` expects argument 1 to be of type `str` '
                'but got type `nil` instead'):
            await client.query(r'has_enum(nil);')

        self.assertTrue(await client.query(r'has_enum("Color");'))
        self.assertFalse(await client.query(r'has_enum("X");'))

    async def test_enum_init(self, client):
        self.assertIs(await client.query(r'''
            set_enum('Color', {
                RED: '#FF0000',
                GREEN: '#00FF00',
                BLUE: '#0000FF'
            });
            '''), None)

        await client.query(r'''
            assert( Color{GREEN} == Color{||'GREEN'} );
            assert( Color{GREEN} == Color("#00FF00") );
            assert( Color{GREEN} == enum('Color', "#00FF00") );
            assert( enum('Color') == Color() );
        ''')

        with self.assertRaisesRegex(
                TypeError,
                r'enum `Color` is expecting a value of type `str` but '
                r'got type `int` instead'):
            await client.query('Color(1);')

        with self.assertRaisesRegex(
                LookupError,
                r'enum `Color` has no member with value `x`'):
            await client.query('Color("x");')

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

        self.assertEqual(len(info), 6)
        self.assertTrue(isinstance(info['enum_id'], int))
        self.assertTrue(isinstance(info['name'], str))
        self.assertTrue(isinstance(info['default'], str))
        self.assertTrue(isinstance(info['created_at'], int))
        self.assertIs(info['modified_at'], None)
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

        self.assertEqual(len(info), 6)
        self.assertTrue(isinstance(info['enum_id'], int))
        self.assertTrue(isinstance(info['name'], str))
        self.assertTrue(isinstance(info['default'], str))
        self.assertTrue(isinstance(info['created_at'], int))
        self.assertIs(info['modified_at'], None)
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

        with self.assertRaisesRegex(
                LookupError,
                r'variable `Color` is undefined'):
            await client.query('Color{1};')

        with self.assertRaisesRegex(
                LookupError,
                r'variable `Color` is undefined'):
            await client.query('Color{"RED"};')

        with self.assertRaisesRegex(
                LookupError,
                r'enum `Unknown` is undefined'):
            await client.query('Unknown{RED};')

        with self.assertRaisesRegex(
                LookupError,
                r'enum `Unknown` is undefined'):
            await client.query('Unknown{||"RED"};')

        with self.assertRaisesRegex(
                TypeError,
                r'enumerator lookup is expecting type `str` but '
                r'got type `int` instead'):
            await client.query('Color{||1};')

        with self.assertRaisesRegex(
                LookupError,
                r'enum `Color` has no member `PURPLE`'):
            await client.query('Color{PURPLE};')

        with self.assertRaisesRegex(
                LookupError,
                r'enum `Color` has no member `PURPLE`'):
            await client.query('Color{||"PURPLE"};')

        self.assertEqual(await client.query('Color{||"RED"};'), "#FF0000")
        self.assertEqual(await client.query('Color{RED};'), "#FF0000")

    async def test_name(self, client):
        self.assertIs(await client.query(r'''
            set_enum('Color', {
                RED: '#FF0000',
                GREEN: '#00FF00',
                BLUE: '#0000FF'
            });
            '''), None)

        with self.assertRaisesRegex(
                LookupError,
                'type `str` has no function `name`'):
            await client.query('"Color".name();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `name` takes 0 arguments but 2 were given'):
            await client.query('Color{RED}.name(1, 2);')

        self.assertEqual(await client.query('Color{RED}.name()'), 'RED')

    async def test_value(self, client):
        self.assertIs(await client.query(r'''
            set_enum('Color', {
                RED: '#FF0000',
                GREEN: '#00FF00',
                BLUE: '#0000FF'
            });
            '''), None)

        with self.assertRaisesRegex(
                LookupError,
                'type `str` has no function `value`'):
            await client.query('"Color".value();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `value` takes 0 arguments but 2 were given'):
            await client.query('Color{RED}.value(1, 2);')

        self.assertEqual(await client.query('Color{RED}.value()'), '#FF0000')

    async def test_is_enum(self, client):
        self.assertIs(await client.query(r'''
            set_enum('Color', {
                RED: '#FF0000',
                GREEN: '#00FF00',
                BLUE: '#0000FF'
            });
            '''), None)

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `is_enum` takes 1 argument but 2 were given'):
            await client.query('is_enum(1, 2);')

        self.assertTrue(await client.query(
            'is_enum( enum("Color", "#FF0000") );'
        ))
        self.assertTrue(await client.query('is_enum( Color{RED} ); '))
        self.assertTrue(await client.query('''
            is_enum({
                color = "GREEN";
                Color{||color};
            });
        '''))

        self.assertFalse(await client.query('is_enum( Color{RED}.value() ); '))
        self.assertFalse(await client.query('is_enum( Color{RED}.name() ); '))
        self.assertFalse(await client.query('is_enum( "RED" );'))
        self.assertFalse(await client.query('is_enum( "#0000FF" );'))

    async def test_enum_type(self, client):
        self.assertIs(await client.query(r'''
            set_enum('Color', {
                RED: '#FF0000',
                GREEN: '#00FF00',
                BLUE: '#0000FF'
            });
            set_type('Brick', {color: 'Color'});
            .b = Brick{};
            nil;
            '''), None)

        self.assertEqual(await client.query(r'''
            .b.color.name()
            '''), 'RED')

    async def test_rename(self, client):
        await client.query(r'''
            set_enum('Color', {
                RED: '#FF0000',
                GREEN: '#00FF00',
                BLUE: '#0000FF'
            });
        ''')

        await client.query(r'''
            .get_color = |i| {
                i = is_int(i) ? i : randint(0, 3);
                i == 0 && return Color{RED};
                i == 1 && return Color{GREEN};
                i == 2 && return Color{BLUE};
                nil;
            }
        ''')

        self.assertEqual(await client.query('.get_color(0);'), '#FF0000')

        with self.assertRaisesRegex(
                LookupError,
                r'enum `Color` already exists'):
            await client.query(r'''
                    rename_enum('Color', 'Color');
                ''')

        with self.assertRaisesRegex(
                LookupError,
                r'enum `unknown` not found'):
            await client.query(r'''
                    rename_enum('unknown', 'Colors');
                ''')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `rename_enum` takes 2 arguments '
                r'but 1 was given'):
            await client.query(r'''rename_enum('Color'); ''')

        with self.assertRaisesRegex(
                TypeError,
                r'function `rename_enum` expects argument 2 to be of '
                r'type `str` but got type `int` instead;'):
            await client.query(r'''rename_enum('Color', 123); ''')

        await client.query(r'''
                rename_enum('Color', 'Colors');
            ''')

        with self.assertRaisesRegex(
                LookupError,
                r'enum `Color` is undefined'):
            self.assertEqual(await client.query('.get_color(0);'), '#FF0000')

        self.assertEqual(await client.query('Colors()'), '#FF0000')

    async def test_enum_map(self, client):
        # feature request, issue #318
        res = await client.query("""//ti
            set_enum('Color', {
                RED: '#FF0000',
                GREEN: '#00FF00',
                BLUE: '#0000FF',
            });
            enum_map('Color');
        """)
        self.assertEqual(res, {
            "RED": "#FF0000",
            "GREEN": "#00FF00",
            "BLUE": "#0000FF"
        })

        with self.assertRaisesRegex(NumArgumentsError, r'takes 1 argument'):
            await client.query('enum_map();')

        with self.assertRaisesRegex(TypeError, r'1 to be of type `str`'):
            await client.query('enum_map(nil);')

        with self.assertRaisesRegex(LookupError, r'enum `X` not found'):
            await client.query('enum_map("X");')


if __name__ == '__main__':
    run_test(TestEnum())

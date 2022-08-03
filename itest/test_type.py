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


class TestType(TestBase):

    title = 'Test type'

    @default_test_setup(num_nodes=2, seed=1, threshold_full_storage=100)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        # add another node for query validation
        await self.node1.join_until_ready(client)

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_wrap_only(self, client):
        self.assertFalse(await client.query(r'''
            try(set_type('X', {arr: '[str?]'}, {1/0}));
            // tests if type is correctly broken down on error
            has_type('X');
        '''))

    async def test_mod_arr(self, client):
        await client.query(r'''
            set_type('X', {arr: '[str?]'});
            x = X{arr: [nil]};
            .x = X{arr: [nil]};
            mod_type('X', 'add', 'arr2', '[str]', [""]);
        ''')

    async def test_set_prop(self, client):
        await client.query(r'''
            set_type('Pet', {});
            set_type('setPet', {animal: '{Pet}'});
            .t = {};
        ''')

        with self.assertRaisesRegex(
                TypeError,
                r'mismatch in type `setPet`; '
                r'property `animal` has definition `{Pet}` but '
                r'got a set with at least one thing of another type'):
            await client.query(r'''
                .t.p = setPet{
                    animal: set({})
                };
            ''')

        with self.assertRaisesRegex(
                TypeError,
                r'type `thing` is not allowed in restricted set'):
            await client.query(r'''
                .t.p = setPet{
                    animal: set()
                };
                .t.p.animal.add({});
            ''')

    async def test_method(self, client):
        await client.query(r'''
            set_type('Person', {
                name: 'str',
                upper: |this| this.name.upper(),
                to_upper: |this| this.name = this.name.upper(),
            });
            .iris = Person{name: 'Iris'};
        ''')

        self.assertEqual(await client.query('.iris.upper();'), 'IRIS')

        with self.assertRaisesRegex(
                OperationError,
                r'closures with side effects require a change but none is '
                r'created; use `wse\(...\)` to enforce a change;'):
            self.assertEqual(await client.query('.iris.to_upper();'), 'Iris')

        self.assertEqual(await client.query('.iris.name;'), 'Iris')
        self.assertEqual(await client.query('wse(.iris.to_upper());'), 'IRIS')
        self.assertEqual(await client.query('.iris.name;'), 'IRIS')
        self.assertEqual(
            await client.query('str(.iris.upper)'),
            '|this| this.name.upper()')

        with self.assertRaisesRegex(
                LookupError,
                r'type `Person` has no property `upper'):
            await client.query('.iris.upper = ||nil;')

        with self.assertRaisesRegex(
                TypeError,
                r'type `nil` has no properties'):
            await client.query('.iris.get("upper").call();')

        self.assertEqual(
            await client.query('.iris["upper"].call(.iris);'),
            'IRIS')

        self.assertEqual(
            await client.query('.iris.get("upper").call(.iris);'),
            'IRIS')

        await client.query('''types_info();''')

        await client.query(r'''
            mod_type('Person', 'mod', 'upper',
                |this| {
                    "convert to upper case";
                    this.name.upper();
                }
            );
        ''')

        await client.query('''types_info();''')

    async def test_new_type(self, client0):

        with self.assertRaisesRegex(
                ValueError,
                'name `datetime` is reserved'):
            await client0.query(r''' set_type('datetime', {}); ''')

        with self.assertRaisesRegex(
                ValueError,
                'name `closure` is reserved'):
            await client0.query(r''' new_type('closure', {}); ''')

        await client0.query(r'''
            set_type('User', {
                name: 'str',
                age: 'uint',
                likes: '[User]?',
            });
            set_type(new_type('People'), {
                users: '[User]'
            });
            set_type(new_type('UserName'), {
                name: 'str',
            });
        ''')

        await client0.query(r'''
            .iris = new('User', {
                name: 'Iris',
                age: 6,
            });
            .cato = User{
                name: 'Cato',
                age: 5,
            };
            .people = new('People', {users: [.iris, .cato]});
            .people.users.push(User{});
        ''')

        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')

        await self.wait_nodes_ready(client0)

        self.assertEqual(await client0.query('.people.users.len();'), 3)
        self.assertEqual(await client1.query('.people.users.len();'), 3)

        client1.close()
        await client1.wait_closed()

    async def test_mod_type_add(self, client0):
        await client0.query(r'''
            set_type(new_type('User'), {
                name: 'str',
                age: 'uint',
                likes: '[User]?',
            });
            set_type('People', {
                users: '[User]'
            });
            set_type('UserName', {
                name: 'str',
                u: 'UserName?',
            });
        ''')

        await client0.query(r'''
            .iris = new('User', {
                name: 'Iris',
                age: 6,
            });
            .cato = User{
                name: 'Cato',
                age: 5,
            };
            .lena = User{
                name: 'Lena',
                age: 6,
            };
            .people = new('People', {users: [.iris, .cato, .lena]});
        ''')

        await client0.query(r'''
            mod_type('User', 'add', 'friend', 'User?', User{
                name: 'Anne',
                age: 5
            });
            mod_type('User', 'del', 'age');
            mod_type('User', 'mod', 'name', 'str?');
        ''')

        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')

        await self.wait_nodes_ready(client0)
        iris_node0 = await client0.query('return .iris, 2;')
        iris_node1 = await client1.query('return .iris, 2;')

        client1.close()
        await client1.wait_closed()

        self.assertEqual(iris_node0, iris_node1)
        self.assertIs(iris_node0.get('age'), None)
        self.assertEqual(iris_node0.get('friend').get('name'), 'Anne')

    async def test_enum_wrap_thing(self, client0):
        await client0.query(r'''
            set_type('TColor', {
                name: 'str',
                code: 'str'
            });
            set_enum('Color', {
                RED: TColor{
                    name: 'red',
                    code: 'ff0000'
                },
                GREEN: TColor{
                    name: 'red',
                    code: '00ff00'
                },
                BLUE: TColor{
                    name: 'blue',
                    code: '0000ff'
                }
            });
            set_type('Brick', {
                part_nr: 'int',
                color: 'Color',
            });

            set_type('Brick2', {
                part_nr: 'int',
                color: 'thing',
            });

            .bricks = [
                Brick{
                    part_nr: 12,
                    color: Color{RED}
                },
                Brick2{
                    part_nr: 13,
                    color: Color{GREEN}.value()
                },
                {
                    part_nr: 14,
                    color: Color{BLUE}
                },
                {
                    part_nr: 15,
                    color: {
                        name: 'magenta',
                        code: 'ff00ff'
                    }
                },
                {
                    part_nr: 16,
                    color: TColor{
                        name: 'yellow',
                        code: 'ffff00'
                    }
                },
            ];

            set_type('_Name', {
                name: 'str'
            });

            set_type('_ColorName', {
                color: '_Name'
            });
        ''')

        brick_color_names = await client0.query(r'''
            return .bricks.map(|b| b.wrap('_ColorName')), 2;
        ''')

        self.assertEqual(len(brick_color_names), 5)
        for brick in brick_color_names:
            self.assertIn('#', brick)
            self.assertIn('color', brick)
            self.assertEqual(len(brick), 2)
            color = brick['color']
            self.assertIn('#', color)
            self.assertIn('name', color)
            self.assertEqual(len(color), 2)

    async def test_enum_wrap_int(self, client0):
        await client0.query(r'''
            set_type('TColor', {
                name: 'str',
                code: 'str'
            });
            set_enum('Color', {
                RED: 0,
                GREEN: 1,
                BLUE: 2,
            });
            set_type('Brick', {
                part_nr: 'int',
                color: 'Color',
            });

            set_type('Brick2', {
                part_nr: 'int',
                color: 'uint',
            });

            .bricks = [
                Brick{
                    part_nr: 12,
                    color: Color{RED}
                },
                Brick2{
                    part_nr: 13,
                    color: Color{GREEN}.value()
                },
                {
                    part_nr: 14,
                    color: Color{BLUE}
                },
                {
                    part_nr: 15,
                    color: 3
                },
            ];

            set_type('_Color', {
                color: 'int'
            });
        ''')

        bricks = await client0.query(r'''
            return .bricks.map(|b| b.wrap('_Color')), 2;
        ''')

        self.assertEqual(len(bricks), 4)
        for brick in bricks:
            self.assertEqual(len(brick), 2)
            self.assertIn('#', brick)
            self.assertIn('color', brick)
            self.assertIsInstance(brick['color'], int)

    async def test_wrap(self, client0):
        only_name = await client0.query(r'''
            set_type('_Name', {name: 'any'});
            .only_name = {
                name: 'Iris',
                age: 6
            }.wrap('_Name');
        ''')

        with self.assertRaisesRegex(
                LookupError,
                r'type `X` not found'):
            await client0.query('''{}.wrap('X');''', scope='@t')

        self.assertEqual(only_name['name'], 'Iris')
        self.assertIn('#', only_name)

        iris = {k: v for k, v in only_name.items()}
        iris['age'] = 6

        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')

        await self.wait_nodes_ready(client0)

        for client in (client0, client1):
            test = await client.query('.only_name;')
            self.assertEqual(test, only_name)

        for client in (client0, client1):
            test = await client.query('.only_name.unwrap();')
            self.assertEqual(test, iris)

        client1.close()
        await client1.wait_closed()

    async def test_circular_dep(self, client):
        await client.query(r'''
            new_type('Tic');
            new_type('Tac');
            new_type('Toe');
        ''')
        await client.query(r'''
            set_type('Tic', {
                tac: 'Tac'
            });
            set_type('Tac', {
                toe: 'Toe'
            });
        ''')
        with self.assertRaisesRegex(
                ValueError,
                r'invalid declaration for `toe` on type `Toe`; '
                r'missing `\?` after declaration `Toe`; '
                r'circular dependencies must be nillable '
                r'at least at one point in the chain'):
            await client.query(r'''
                set_type('Toe', {
                    toe: 'Toe'
                });
            ''')
        with self.assertRaisesRegex(
                ValueError,
                r'invalid declaration for `tic` on type `Toe`; '
                r'missing `\?` after declaration `Tic`; '
                r'circular dependencies must be nillable '
                r'at least at one point in the chain'):
            await client.query(r'''
                set_type('Toe', {
                    tic: 'Tic'
                });
            ''')
        await client.query(r'''
            set_type('Toe', {
                tic: 'Tic?'
            });
        ''')
        with self.assertRaisesRegex(
                ValueError,
                r'invalid declaration for `alt` on type `Toe`; '
                r'missing `\?` after declaration `Tic`; '
                r'circular dependencies must be nillable '
                r'at least at one point in the chain'):
            await client.query(r'''
                mod_type('Toe', 'add', 'alt', 'Tic');
            ''')
        await client.query(r'''
            mod_type('Tac', 'mod', 'toe', 'Toe?');
        ''')
        await client.query(r'''
            mod_type('Toe', 'add', 'alt', 'Tic');
        ''')

    async def test_mod_del(self, client):
        await client.query(r'''
            new_type('Foo');
            set_type('Foo', {
                foo: 'Foo?',
            });
            .arr = [
                1, 2, 3, 4, 5, 6, 7, 8, 9, 10,11, 12, 13, 14, 15,
                16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28
            ].map(|| Foo{});
            .arr.map(|foo, i| foo.foo = .arr[i-1]);
            .del('arr');
            nil;
        ''')

        await client.query(r'''
            mod_type('Foo', 'del', 'foo');
        ''')

    async def test_mod(self, client):
        await client.query(r'''
            set_type('Person', {
                name: 'str',
                age: 'uint',
                ucase: |this| this.name.ucase(),
            });
            .iris = Person{name: 'Iris', age: 7};
        ''')

        with self.assertRaisesRegex(
                LookupError,
                'type `str` has no function `mod_type`'):
            await client.query('"Color".mod_type();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `mod_type` requires at least 3 arguments '
                'but 0 were given'):
            await client.query('mod_type();')

        with self.assertRaisesRegex(
                TypeError,
                'function `mod_type` expects argument 1 to be of type `str` '
                'but got type `nil` instead'):
            await client.query(r'mod_type(nil, nil, nil);')

        with self.assertRaisesRegex(
                LookupError,
                r'type `X` not found'):
            await client.query(r'mod_type("X", nil, nil);')

        with self.assertRaisesRegex(
                ValueError,
                r'type name must follow the naming rules'):
            await client.query(r'mod_type("", nil, nil);')

        with self.assertRaisesRegex(
                TypeError,
                r'function `mod_type` expects argument 2 to be of '
                r'type `str` but got type `nil` instead'):
            await client.query(r'mod_type("Person", nil, nil);')

        with self.assertRaisesRegex(
                ValueError,
                r'function `mod_type` expects argument 2 to be `add`, `del`, '
                r'`mod`, `rel`, `ren` or `wpo` but got `x` instead'):
            await client.query(r'mod_type("Person", "x", "x");')

        # section ADD
        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `mod_type` with task `add` requires at least 4 '
                r'arguments but 3 were given'):
            await client.query(r'mod_type("Person", "add", "x");')

        with self.assertRaisesRegex(
                TypeError,
                r'function `mod_type` with task `add` expects argument 4 to '
                r'be of type `str` or type `closure` but got type `nil` '
                r'instead'):
            await client.query(r'''
                mod_type("Person", "add", "hair_type", nil, nil);
            ''')

        with self.assertRaisesRegex(
                OperationError,
                r'field `hair_type` is added to type `Person` but at least '
                r'one error has occurred using the given callback; mismatch '
                r'in type `Person`; type `int` is invalid for '
                r'property `hair_type` with definition `str'):
            await client.query(r'''
                mod_type("Person", "add", "hair_type", "str", ||4);
            ''')

        await client.query(r'''
            mod_type("Person", "del", "hair_type");
        ''')

        with self.assertRaisesRegex(
                TypeError,
                r'invalid declaration for `hair_type` on type `Person`; '
                r'unknown type `invalid` in declaration'):
            await client.query(r'''
                mod_type("Person", "add", "hair_type", "invalid", nil);
            ''')

        with self.assertRaisesRegex(
                TypeError,
                r'mismatch in type `Person`; type `nil` is invalid '
                r'for property `hair_type` with definition `str'):
            await client.query(r'''
                mod_type("Person", "add", "hair_type", "str", nil);
            ''')

        with self.assertRaisesRegex(
                LookupError,
                r'property or method `age` already exists on type `Person`'):
            await client.query(r'''
                mod_type("Person", "add", "age", "str", 'blonde');
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'function `mod_type` expects argument 3 to follow '
                r'the naming rules'):
            await client.query(r'''
                mod_type("Person", "add", "!hair_type", "str", 'blonde');
            ''')

        with self.assertRaisesRegex(
                OperationError,
                r'cannot change type `Person` while the type '
                r'is in use'):
            await client.query(r'''
                mod_type("Person", "add", "hair_type", {
                    mod_type("Person", "add", "hair_type", "str", "blonde");
                    "str";
                }, "blonde");
            ''')

        self.assertIs(await client.query(r'''
                mod_type("Person", "add", "hair_type", "str", "blonde");
            '''), None)

        self.assertEqual(await client.query(r'.iris.hair_type;'), "blonde")

        await client.query(r'''
                mod_type("Person", "add", "birthday", |this| {
                    this.age += 1;
                    `Hooray, {this.name} is {this.age} years old!`
                });
            ''')

        self.assertEqual(
            await client.query(r'wse(.iris.birthday());'),
            "Hooray, Iris is 8 years old!")

        # section MOD
        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `mod_type` with task `mod` requires at '
                r'least 4 arguments but 3 were give'):
            await client.query(r'mod_type("Person", "mod", "x");')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `mod_type` with task `mod` takes at '
                r'most 5 arguments but 6 were give'):
            await client.query(r'mod_type("Person", "mod", "x", 0, 0, 0);')

        with self.assertRaisesRegex(
                LookupError,
                r'type `Person` has no property or method `x'):
            await client.query(r'mod_type("Person", "mod", "x", "#EEEE11");')

        with self.assertRaisesRegex(
                OperationError,
                r'cannot apply type declaration `int` to `hair_type` on '
                r'type `Person` without a closure to migrate existing '
                r'instances; the old declaration `str` is not compatible '
                r'with the new declaration'):
            await client.query(r'''
                mod_type("Person", "mod", "hair_type", "int");
            ''')

        self.assertIs(await client.query(r'''
                mod_type("Person", "mod", "hair_type", "any");
            '''), None)

        with self.assertRaisesRegex(
                TypeError,
                r'function `mod_type` with task `mod` expects argument 5 to '
                r'be of type `closure` but got type `str` instead;'):
            await client.query(r'''
                mod_type("Person", "mod", "hair_type", "str", "");
            ''')

        self.assertEqual(await client.query(r'.iris.hair_type;'), "blonde")

        await client.query(r'''
            mod_type("Person", "mod", "hair_type", "str", |p| {
                p.hair_type.upper();
            });
        ''')

        self.assertEqual(await client.query(r'.iris.hair_type;'), "BLONDE")

        await client.query(r'''
            mod_type("Person", "mod", "hair_type", "utf8", |p| {
                p.hair_type = p.hair_type.lower();
                nil;
            });
        ''')

        self.assertEqual(await client.query(r'.iris.hair_type;'), "blonde")

        with self.assertRaisesRegex(
                OperationError,
                r'field `hair_type` on type `Person` is modified but at least '
                r'one error has occurred using the given callback: '
                r'mismatch in type `Person`; type `int` is invalid for '
                r'property `hair_type` with definition `str`'):
            await client.query(r'''
                mod_type("Person", "mod", "hair_type", "str", || {
                    1;
                });
            ''')

        self.assertEqual(await client.query(r'.iris.hair_type;'), "blonde")

        self.assertIs(await client.query(r'''
                mod_type("Person", "mod", "hair_type", "any");
                .tess = Person{name: 'Tess', hair_type: 123};
                nil;
            '''), None)

        with self.assertRaisesRegex(
                TypeError,
                r'cannot convert a property into a method'):
            await client.query(r'''
                mod_type("Person", "mod", "hair_type", ||nil);
            ''')

        with self.assertRaisesRegex(
                TypeError,
                r'cannot convert a method into a property'):
            await client.query(r'''
                mod_type("Person", "mod", "birthday", 'str');
            ''')

        self.assertEqual(await client.query(r'.iris.hair_type;'), "blonde")
        self.assertEqual(await client.query(r'.tess.hair_type;'), 123)

        with self.assertRaisesRegex(
                OperationError,
                r'field `hair_type` on type `Person` is modified but at least '
                r'one failed attempt was made to keep the original value: '
                r'mismatch in type `Person`; type `int` is invalid for '
                r'property `hair_type` with definition `str`'):
            await client.query(r'''
                mod_type("Person", "mod", "hair_type", "str", |p| {
                    nil;
                });
            ''')

        self.assertEqual(await client.query(r'.iris.hair_type;'), "blonde")
        self.assertEqual(await client.query(r'.tess.hair_type;'), "")

        await client.query(r'''
                mod_type("Person", "mod", "birthday", |this, when| {
                    `The birthday of {this.name} is on {when}!`
                });
            ''')

        self.assertEqual(
            await client.query(r'.tess.birthday("Monday");'),
            "The birthday of Tess is on Monday!")

        # Section REN
        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `mod_type` with task `ren` takes 4 arguments '
                r'but 3 were given'):
            await client.query(r'mod_type("Person", "ren", "x");')

        with self.assertRaisesRegex(
                LookupError,
                r'type `Person` has no property or method `x'):
            await client.query(r'mod_type("Person", "ren", "x", "y");')

        with self.assertRaisesRegex(
                TypeError,
                r'function `mod_type` with task `ren` expects argument 4 to '
                r'be of type `str` but got type `int` instead'):
            await client.query(r'mod_type("Person", "ren", "hair_type", 1);')

        with self.assertRaisesRegex(
                ValueError,
                r'property name must follow the naming rules'):
            await client.query(r'''
                mod_type("Person", "ren", "hair_type", "!hair");
            ''')

        with self.assertRaisesRegex(
                ValueError,
                r'property or method `age` already exists on type `Person`'):
            await client.query(r'''
                mod_type("Person", "ren", "hair_type", "age");
            ''')

        self.assertIs(await client.query(r'''
                mod_type("Person", "ren", "hair_type", "hair");
            '''), None)

        self.assertEqual(await client.query(r'.iris.hair;'), "blonde")

        with self.assertRaisesRegex(
                ValueError,
                r'method name must follow the naming rules'):
            await client.query(r'''
                    mod_type("Person", "ren", "birthday", "happy bithday");
                ''')

        await client.query(r'''
                mod_type("Person", "ren", "birthday", "happy_birthday");
            ''')

        self.assertEqual(
            await client.query(r'.iris.happy_birthday("Tuesday");'),
            "The birthday of Iris is on Tuesday!")

        # Section DEL
        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `mod_type` with task `del` takes 3 arguments '
                r'but 4 were given'):
            await client.query(r'mod_type("Person", "del", "x", "y");')

        with self.assertRaisesRegex(
                ValueError,
                r'function `mod_type` expects argument 3 to '
                r'follow the naming rules'):
            await client.query(r'mod_type("Person", "del", "!");')

        with self.assertRaisesRegex(
                LookupError,
                r'type `Person` has no property or method `x`'):
            await client.query(r'mod_type("Person", "del", "x");')

        self.assertIs(await client.query(r'''
                mod_type("Person", "del", "hair");
            '''), None)

        with self.assertRaisesRegex(
                LookupError,
                r'type `Person` has no property or method `hair`'):
            await client.query(r'.iris.hair;')

        self.assertIs(await client.query(r'''
                mod_type("Person", "del", "happy_birthday");
            '''), None)

        with self.assertRaisesRegex(
                LookupError,
                r'type `Person` has no property or method `happy_birthday`'):
            await client.query(r'.iris.happy_birthday();')

        # Section WPO
        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `mod_type` with task `wpo` takes 3 arguments '
                r'but 4 were given'):
            await client.query(r'mod_type("Person", "wpo", true, "x");')

        with self.assertRaisesRegex(
                TypeError,
                r'function `mod_type` with task `wpo` expects argument 3 to '
                r'be of type `bool` but got type `int` instead'):
            await client.query(r'mod_type("Person", "wpo", 1);')

        with self.assertRaisesRegex(
                OperationError,
                r'a type can only be changed to `wrap-only` mode without '
                r'having active instances; 2 active instances of '
                r'type `Person` have been found'):
            await client.query(r'mod_type("Person", "wpo", true);')

    async def test_del_type(self, client):
        await client.query(r'''
            new_type('Tic');
            new_type('Tac');
            new_type('Toe');
            new_type('Foo');
            new_type('Bar');

            set_type('Tic', {
                tic: '[Tic]'
            });
            set_type('Tac', {
                tac: '{Tac}'
            });
            set_type('Toe', {
                Toe: 'Toe?'
            });
            set_type('Foo', {
                foo: 'Foo?'
            });
            set_type('Bar', {
                bar: '[Bar]'
            });
            .tic = Tic{tic: []};
            .tac = Tac{tac: {}};
            .toe = Toe{};
            .foo = Foo{};
            .bar = Bar{bar: []};
        ''')

        await client.query(r'''
            mod_type('Foo', 'del', 'foo');
            mod_type('Bar', 'del', 'bar');
        ''')

        with self.assertRaisesRegex(
                LookupError,
                r'type `Foo` has no property or method `card`'):
            await client.query(r'''
                mod_type('Foo', 'del', 'card');
            ''')

        await client.query(r'''
            del_type('Tic');
            del_type('Tac');
            del_type('Toe');
            del_type('Foo');
            del_type('Bar');
        ''')

    async def test_type_mismatch(self, client):
        await client.query(r'''
            new_type('Tic');
            new_type('Tac');
            new_type('Toe');

            set_type('Tic', {
                tac: '[Tac]'
            });

            .tic = Tic{
                tac: []
            };
        ''')

        with self.assertRaisesRegex(
                TypeError,
                r'mismatch in type `Tic`; '
                r'property `tac` requires an array with items that '
                r'matches definition `\[Tac\]`'):
            await client.query(r'''
                tic = Tic{
                    tac: [{
                        foo: 42
                    }]
                };
            ''')

        with self.assertRaisesRegex(
                TypeError,
                r'type `thing` is not allowed in restricted array'):
            await client.query(r'''
                .tic.tac.push({
                    foo: 1
                });
            ''')

    async def test_migrate(self, client0):

        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')

        await client0.query(r'''
            set_type('User', {
                name: 'str'
            });
            .users = [
                User{name: 'Iris'},
                User{name: 'Cato'},
                User{name: 'Anna'},
            ];
            set_type('Color', {
                name: 'str'
            });
            .colors = [
                Color{name: 'red'},
                Color{name: 'green'},
                Color{name: 'blue'},
            ];
            mod_type('User', 'add', 'color', 'Color', Color{name: 'dummy'});
            .users.each(|u| {
                u.color = .colors.choice();
            });
        ''')

        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')

    async def test_sync_add(self, client0):
        await client0.query(r'''
            set_type('Book', {
                title: 'str',
            });
            set_type('Books', {
                own: '[Book]',
                fav: 'Book?',
                unique: '{Book}'
            });
            .books = Books{
                own: [],
                unique: set(),
            };
        ''')

        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')

        await self.wait_nodes_ready(client0)

        for client in (client0, client1):
            with self.assertRaisesRegex(
                    TypeError,
                    r'type `thing` is not allowed in restricted array'):
                await client.query(r'''
                    .books.own.push({
                        foo: 'Not a book'
                    });
                ''')

            with self.assertRaisesRegex(
                    TypeError,
                    r'type `Books` is not allowed in restricted array'):
                await client.query(r'''
                    .books.own.push(Books());
                ''')

            with self.assertRaisesRegex(
                    TypeError,
                    r'mismatch in type `Books`; type `thing` is invalid '
                    r'for property `fav` with definition `Book\?`'):
                await client.query(r'''
                    .books.fav = {
                        foo: 'Not a book'
                    };
                ''')

            with self.assertRaisesRegex(
                    TypeError,
                    r'type `thing` is not allowed in restricted set'):
                await client.query(r'''
                    .books.unique.add({
                        foo: 'Not a book'
                    });
                ''')

            with self.assertRaisesRegex(
                    TypeError,
                    r'type `Books` is not allowed in restricted set'):
                await client.query(r'''
                    .books.unique.add(Books());
                ''')

        books0 = await client0.query(r'.books.filter(||true);')
        books1 = await client1.query(r'.books.filter(||true);')

        client1.close()
        await client1.wait_closed()

        self.assertEqual(books0, {"own": [], "fav": None, "unique": []})
        self.assertEqual(books1, {"own": [], "fav": None, "unique": []})

    async def test_circular_del(self, client):
        await client.query(r'''
            new_type('Foo');
            new_type('Bar');

            set_type('Foo', {
                bar: '[Bar]'
            });

            set_type('Bar', {
                foo: 'Foo'
            });
        ''')

        res = await client.query(r'''
            mod_type('Foo', 'del', 'bar');
            del_type('Bar');
        ''')
        self.assertIs(res, None)

        res = await client.query(r'''
            del_type('Foo');
        ''')
        self.assertIs(res, None)

    async def test_del_type(self, client):
        await client.query(r'''
            new_type('Tic');
            new_type('Tac');
            new_type('Toe');
            set_type('Tic', {tac: 'Tac?', toe: 'Toe?'});
            set_type('Tac', {tic: '[Tic]', toe: '[Toe]'});
            set_type('Toe', {tic: 'Tic?', tac: 'Tac?'});
        ''')

        with self.assertRaisesRegex(
                OperationError,
                r'type `Toe` is used by at least one other type; '
                r'use `types_info\(..\)` to find all dependencies and remove '
                r'them by using `mod_type\(..\)` or delete the dependency '
                r'types as well'):
            await client.query(r'''
                del_type('Toe');
            ''')

        await client.query(r'''
            mod_type('Tic', 'del', 'toe');
            mod_type('Tac', 'mod', 'toe', 'any');
        ''')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `mod_type` with task `mod` requires at '
                r'least 4 arguments but 3 were given'):
            await client.query(r'''
                mod_type('Tac', 'mod', 'card');
            ''')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `mod_type` with task `del` takes 3 arguments '
                r'but 4 were given'):
            await client.query(r'''
                mod_type('Tac', 'del', 'xxx', 5);
            ''')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `mod_type` requires at least 3 arguments '
                r'but 2 were given'):
            await client.query(r'''
                mod_type('Tac', 'del');
            ''')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `mod_type` with task `add` requires at least 4 '
                r'arguments but 3 were given'):
            await client.query(r'''
                mod_type('Tac', 'add', 'xxx');
            ''')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `mod_type` with task `add` takes at most 5 '
                r'arguments but 6 were given'):
            await client.query(r'''
                mod_type('Tac', 'add', 'xxx', 'str', 0, 0);
            ''')

        with self.assertRaisesRegex(
                LookupError,
                r'type `Tac` has no property or method `card`'):
            await client.query(r'''
                mod_type('Tac', 'mod', 'card', 'any');
            ''')

        with self.assertRaisesRegex(
                LookupError,
                r'property or method `toe` already exists on type `Tac`'):
            await client.query(r'''
                mod_type('Tac', 'add', 'toe', 'any');
            ''')

        await client.query(r'''
            del_type('Toe');
        ''')

    async def test_type_definitions(self, client):
        await client.query(r'''
            set_type('_Str', {test: 'str'});
            set_type('_Utf8', {test: 'utf8'});
            set_type('_Raw', {test: 'raw'});
            set_type('_Bytes', {test: 'bytes'});
            set_type('_Bool', {test: 'bool'});
            set_type('_Int', {test: 'int'});
            set_type('_Uint', {test: 'uint'});
            set_type('_Pint', {test: 'pint'});
            set_type('_Nint', {test: 'nint'});
            set_type('_Float', {test: 'float'});
            set_type('_Number', {test: 'number'});
            set_type('_Datetime', {test: 'datetime'});
            set_type('_Timeval', {test: 'timeval'});
            set_type('_Regex', {test: 'regex'});
            set_type('_Closure', {test: 'closure'});
            set_type('_Error', {test: 'error'});
            set_type('_Room', {test: 'room'});
            set_type('_Thing', {test: 'thing'});
            set_type('_Type', {test: '_Str'});
            set_type('_Array', {test: '[]'});
            set_type('_Set', {test: '{}'});
            set_type('_r_array', {test: '[str]'});
            set_type('_r_set', {test: '{_Str}'});
            set_type('_o_str', {test: 'str?'});
            set_type('_o_array', {test: '[str?]'});
            set_type('_Any', {test: 'any'});
        ''')

        with self.assertRaisesRegex(
                TypeError,
                r'mismatch in type `_Str`; '
                r'type `bytes` is invalid for property `test` '
                r'with definition `str`'):
            await client.query(r'''_Str{test: bytes('')};''')

        self.assertEqual(
            await client.query(r'_Str{test: "x"}.test;'), 'x')
        self.assertEqual(
            await client.query(r'_Str{test: "x"}.wrap("_Utf8");'), {})
        self.assertEqual(
            await client.query(r'_Str{test: "x"}.wrap("_Raw");'),
            {'test': 'x'})
        self.assertEqual(
            await client.query(r'_Str{test: "x"}.wrap("_Any");'),
            {'test': 'x'})

        with self.assertRaisesRegex(
                ValueError,
                r'mismatch in type `_Uint`; '
                r'property `test` only accepts integer values '
                r'greater than or equal to 0'):
            await client.query(r'''_Uint{test: -1};''')

        with self.assertRaisesRegex(
                TypeError,
                r'mismatch in type `_Uint`; '
                r'type `nil` is invalid for property `test` '
                r'with definition `uint`'):
            await client.query(r'''_Uint{test: nil};''')

        self.assertEqual(
            await client.query(r'_Uint{test: 0}.test;'), 0)
        self.assertEqual(
            await client.query(r'_Uint{test: 0}.wrap("_Pint");'), {})
        self.assertEqual(
            await client.query(r'_Uint{test: 0}.wrap("_Int");'), {'test': 0})

        with self.assertRaisesRegex(
                ValueError,
                r'mismatch in type `_Pint`; '
                r'property `test` only accepts positive integer values'):
            await client.query(r'''_Pint{test: 0};''')

        with self.assertRaisesRegex(
                TypeError,
                r'mismatch in type `_Pint`; '
                r'type `str` is invalid for property `test` '
                r'with definition `pint`'):
            await client.query(r'''_Pint{test: '0'};''')

        with self.assertRaisesRegex(
                TypeError,
                r'mismatch in type `_Datetime`; '
                r'type `timeval` is invalid for property `test` '
                r'with definition `datetime`'):
            await client.query(r'''_Datetime{test: timeval()};''')

        with self.assertRaisesRegex(
                TypeError,
                r'mismatch in type `_Timeval`; '
                r'type `datetime` is invalid for property `test` '
                r'with definition `timeval`'):
            await client.query(r'''_Timeval{test: datetime()};''')

        with self.assertRaisesRegex(
                TypeError,
                r'mismatch in type `_Regex`; '
                r'type `str` is invalid for property `test` '
                r'with definition `regex`'):
            await client.query(r'''_Regex{test: '\.\*'};''')

        with self.assertRaisesRegex(
                TypeError,
                r'mismatch in type `_Closure`; '
                r'type `str` is invalid for property `test` '
                r'with definition `closure`'):
            await client.query(r'''_Closure{test: '\|\|nil'};''')

        with self.assertRaisesRegex(
                TypeError,
                r'mismatch in type `_Error`; '
                r'type `str` is invalid for property `test` '
                r'with definition `error`'):
            await client.query(r'''_Error{test: 'err'};''')

        with self.assertRaisesRegex(
                TypeError,
                r'mismatch in type `_Room`; '
                r'type `str` is invalid for property `test` '
                r'with definition `room`'):
            await client.query(r'''_Room{test: 'err'};''')

        self.assertEqual(
            await client.query(r'type(_Regex{}.test);'), 'regex')
        self.assertEqual(
            await client.query(r'_Regex{}.test.test("bla");'), True)
        self.assertEqual(
            await client.query(r'_Closure{}.test();'), None)
        self.assertEqual(
            await client.query(r'_Closure{test: ||42}.test();'), 42)
        self.assertEqual(
            await client.query(r'_Error{test:err(-110)}.test.code();'), -110)
        self.assertEqual(
            await client.query(r'_Room{test:room()}.test.emit("test");'), None)

        self.assertEqual(
            await client.query(r'_Pint{test:42}.test;'), 42)
        self.assertEqual(
            await client.query(r'_Pint{test:42}.wrap("_Nint");'), {})
        self.assertEqual(
            await client.query(r'_Pint{test:42}.wrap("_Uint");'), {'test': 42})

        with self.assertRaisesRegex(
                ValueError,
                r'mismatch in type `_Nint`; '
                r'property `test` only accepts negative integer values'):
            await client.query(r'''_Nint{test: 0};''')

        with self.assertRaisesRegex(
                TypeError,
                r'mismatch in type `_Nint`; '
                r'type `str` is invalid for property `test` '
                r'with definition `nint`'):
            await client.query(r'''_Nint{test: '0'};''')

        self.assertEqual(
            await client.query(r'_Nint{test:-6}.test;'), -6)
        self.assertEqual(
            await client.query(r'_Nint{test:-6}.wrap("_Uint");'), {})
        self.assertEqual(
            await client.query(r'_Nint{test:-6}.wrap("_Number")'),
            {'test': -6})

    async def test_mod_type_add_closure_and_ts(self, client0):
        now = int(time.time())

        await client0.query(r'''
            set_type('Chat', {
                messages: '[str]'
            });
            set_type('Room', {
                name: 'str',
            });
            .room_a = Room{name: 'room A'};
            .room_b = Room{name: 'room B'};
            mod_type('Room', 'add', 'chat', 'Chat');
            mod_type('Room', 'add', 'ts', 'timeval');

            .room_a.chat.messages.push('Just one instance');
        ''')

        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')

        await self.wait_nodes_ready(client0)

        ts_a = await client0.query('.room_a.ts;')
        self.assertIsInstance(ts_a, int)
        self.assertAlmostEqual(ts_a, now, delta=1)

        ts_b = await client0.query('.room_a.ts;')
        self.assertIsInstance(ts_b, int)
        self.assertAlmostEqual(ts_b, now, delta=1)

        for client in (client0, client1):
            msg = await client.query('.room_a.chat.messages[0];')
            self.assertEqual(msg, 'Just one instance')

            msg = await client.query('.room_b.chat.messages[0];')
            self.assertEqual(msg, 'Just one instance')

            self.assertEqual(await client.query('.room_a.ts;'), ts_a)
            self.assertEqual(await client.query('.room_b.ts;'), ts_b)

        await client0.query(r'''
            mod_type('Room', 'del', 'chat');
            mod_type('Room', 'add', 'chat', 'Chat', |room| {
                Chat{messages: [`Welcome in {room.name}`]}
            });
            mod_type('Room', 'mod', 'ts', 'datetime', |r| datetime(r.ts));
        ''')

        await self.wait_nodes_ready(client0)

        dt_a = await client0.query('.room_a.ts;')
        self.assertIsInstance(dt_a, str)

        dt_b = await client0.query('.room_a.ts;')
        self.assertIsInstance(dt_b, str)

        for client in (client0, client1):
            msg = await client.query('.room_a.chat.messages[0];')
            self.assertEqual(msg, 'Welcome in room A')

            msg = await client.query('.room_b.chat.messages[0];')
            self.assertEqual(msg, 'Welcome in room B')

            self.assertEqual(await client.query('int(.room_a.ts)'), ts_a)
            self.assertEqual(await client.query('int(.room_b.ts)'), ts_b)

        client1.close()
        await client1.wait_closed()

    async def test_mod_type_mod_closure(self, client0):
        await client0.query(r'''
            set_type('Chat', {
                name: 'str',
                messages: '[str]'
            });
            set_type('Room', {
                chat: 'str',
            });
            .room_a = Room{chat: 'room A'};
            .room_b = Room{chat: 'room B'};

            try(mod_type('Room', 'mod', 'chat', 'Chat', || nil));

            .room_a.chat.messages.push('Just one instance');
        ''')

        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')

        await self.wait_nodes_ready(client0)

        for client in (client0, client1):
            msg = await client.query('.room_a.chat.messages[0];')
            self.assertEqual(msg, 'Just one instance')

            msg = await client.query('.room_b.chat.messages[0];')
            self.assertEqual(msg, 'Just one instance')

            name = await client.query('.room_a.chat.name;')
            self.assertEqual(name, '')

            name = await client.query('.room_b.chat.name;')
            self.assertEqual(name, '')

        await client0.query(r'''
            mod_type('Room', 'mod', 'chat', 'str', ||'');
            .room_a.chat = 'room A';
            .room_b.chat = 'room B';
        ''')

        await client0.query(r'''
            mod_type('Room', 'mod', 'chat', 'Chat', |room| Chat{
                name: room.chat,
                messages: [`Welcome in {room.chat}`]
            });

            .room_a.chat.messages.push('Just one instance');
        ''')

        await self.wait_nodes_ready(client0)

        for client in (client0, client1):
            msg = await client.query('.room_a.chat.messages[0];')
            self.assertEqual(msg, 'Welcome in room A')

            msg = await client.query('.room_b.chat.messages[0];')
            self.assertEqual(msg, 'Welcome in room B')

            name = await client.query('.room_a.chat.name;')
            self.assertEqual(name, 'room A')

            name = await client.query('.room_b.chat.name;')
            self.assertEqual(name, 'room B')

        client1.close()
        await client1.wait_closed()

    async def test_mod_type_mod_advanced0(self, client0):
        with self.assertRaisesRegex(
                OperationError,
                r'field `chat` is added to type `Room` but at least one '
                r'error has occurred using the given callback; mismatch in '
                r'type `Room`; type `int` is invalid for property `chat` with '
                r'definition `Room\?`'):
            await client0.query(r'''
                set_type('Chat', {
                    messages: '[str]'
                });

                set_type('Room', {
                    name: 'str',
                });

                .room_a = Room{name: 'room A'};
                .room_b = Room{name: 'room B'};

                mod_type('Room', 'add', 'chat', 'Room?', |room| {
                    Room{name: `sub{room.name}`, chat: 123};
                });
            ''')

        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')

        await self.wait_nodes_ready(client0)

        for client in (client0, client1):
            self.assertIs(await client.query('.room_a.chat;'), None)
            self.assertIs(await client.query('.room_b.chat;'), None)

        client1.close()
        await client1.wait_closed()

    async def test_mod_type_mod_advanced1(self, client0):
        await client0.query(r'''
            set_type('Chat', {
                messages: '[str]'
            });

            set_type('Room', {
                name: 'str',
            });

            .room_a = Room{name: 'room A'};
            .room_b = Room{name: 'room B'};

            mod_type('Room', 'add', 'chat', 'Room?', |room| {
                Room{name: `sub{room.name}`};
            });
        ''')

        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')

        await self.wait_nodes_ready(client0)

        for client in (client0, client1):
            msg = await client.query('.room_a.chat.name;')
            self.assertEqual(msg, 'subroom A')

            msg = await client.query('.room_b.chat.name;')
            self.assertEqual(msg, 'subroom B')

            self.assertIs(await client.query('.room_a.chat.chat;'), None)
            self.assertIs(await client.query('.room_b.chat.chat;'), None)

        await client0.query(r'''
            room_c = Room{};

            mod_type('Room', 'mod', 'chat', 'Chat', |room| Chat{
                messages: [`Welcome in {room.name}`]
            });
        ''')

        client1.close()
        await client1.wait_closed()

    async def test_mod_type_mod_advanced2(self, client0):
        with self.assertRaisesRegex(
                OperationError,
                r'field `chat` on type `Room` is modified but at least one '
                r'instance got an inappropriate value from the migration '
                r'callback; to be compliant, ThingsDB has used the default '
                r'value for this instance; callback response: mismatch in '
                r'type `Room`; type `int` is invalid for property `chat` with '
                r'definition `Room\?`'):
            await client0.query(r'''
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

        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')

        await self.wait_nodes_ready(client0)

        for client in (client0, client1):
            msg = await client.query('.room_a.chat.name;')
            self.assertEqual(msg, 'subroom A')

            msg = await client.query('.room_b.chat.name;')
            self.assertEqual(msg, 'subroom B')

            self.assertIs(await client.query('.room_a.chat.chat;'), None)
            self.assertIs(await client.query('.room_b.chat.chat;'), None)

        client1.close()
        await client1.wait_closed()

    async def test_wpo(self, client0):
        await client0.query(r'''
            new_type('A', true);
            new_type('B', false);
            mod_type('B', 'wpo', true);
            new_type('C', true);
            mod_type('C', 'wpo', false);
            set_type('D', {}, true);
            set_type('E', {}, false);
        ''')

        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')

        await asyncio.sleep(1.6)

        for client in (client0, client1):
            res = await client.query(r'''
                type_assert(try(A{}), 'error');
                type_assert(try(B{}), 'error');
                type_assert(try(C{}), 'C');
                type_assert(try(D{}), 'error');
                type_assert(try(E{}), 'E');
            ''')

        await client0.query(r'''
            mod_type('A', 'add', 'x', 'B');
        ''')

        await client0.query(r'''
            mod_type('A', 'mod', 'x', 'C');
        ''')

        await asyncio.sleep(1.6)

        client1.close()
        await client1.wait_closed()

    async def test_err_handling(self, client0):
        with self.assertRaisesRegex(
                TypeError,
                r'invalid declaration for `x` on type `A`; '
                r'type `set` cannot contain a nillable type;'):
            await client0.query(r'''
                new_type('A');
                new_type('B');
                set_type('A', {
                    a: 'int',
                    x: '{B?}'
                });
            ''')

        # type A should be created as an empty type
        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')

        await asyncio.sleep(1.6)

        for client in (client0, client1):
            self.assertEqual(await client.query('A{}'), {})

        client1.close()
        await client1.wait_closed()

    async def test_init_create(self, client0):
        await client0.query(r'''
            set_type('X', {
                a: new_type('A'),
                b: new_type('B'),
            });
        ''')

        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')

        await asyncio.sleep(1.6)

        for client in (client0, client1):
            self.assertEqual(await client.query('X{}'), {'a': {}, 'b': {}})

        client1.close()
        await client1.wait_closed()

    async def test_rename(self, client0):
        await client0.query(r'''
            set_type('A', {i: 'int'});
            set_type('B', {a: 'A'});
        ''')

        await client0.query(r'''
            .a = A{};
            .b = B{};
        ''')

        with self.assertRaisesRegex(
                LookupError,
                r'type `B` already exists'):
            await client0.query(r'''
                    rename_type('A', 'B');
                ''')

        with self.assertRaisesRegex(
                LookupError,
                r'type `unknown` not found'):
            await client0.query(r'''
                    rename_type('unknown', 'A');
                ''')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `rename_type` takes 2 arguments '
                r'but 1 was given'):
            await client0.query(r'''rename_type('A'); ''')

        with self.assertRaisesRegex(
                TypeError,
                r'function `rename_type` expects argument 2 to be of '
                r'type `str` but got type `int` instead;'):
            await client0.query(r'''rename_type('A', 123); ''')

        await client0.query(r'''
                a = A{}; b = B{};
                rename_type('A', 'TypeA');
                rename_type('B', 'TypeB');
            ''')

        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')

        await asyncio.sleep(1.6)

        for client in (client0, client1):
            self.assertEqual(await client.query(r'''
                [TypeA{}, TypeB{}]
            '''), [{'i': 0}, {'a': {}}])

        client1.close()
        await client1.wait_closed()

    async def test_rename_prop(self, client0):
        await client0.query(r'''
            set_type('Test', {
                arr: '[]',
                set: '{}',
                oarr: '[]?',
                oset: '{}?',
            });

            .test1 = Test{
                arr: range(10),
                set: set(),
            };
            .test2 = Test{
                oarr: range(1),
                oset: set({})
            };
        ''')

        await client0.query(r'''
            t = Test{set: set({}, {})};
            mod_type('Test', 'ren', 'arr', 'list');
            mod_type('Test', 'ren', 'oarr', 'olist');
            mod_type('Test', 'ren', 'set', 'col');
            mod_type('Test', 'ren', 'oset', 'ocol');
            .test3 = t;
        ''')

        await client0.query(r'''
            .test1.list.push(10);
            .test1.col.add({});
            .test2.olist.push(42);
            .test2.ocol.add({});
            .test3.list.push(123);
            .test3.col.add({});
        ''')

        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')

        await asyncio.sleep(1.6)

        for client in (client0, client1):
            self.assertEqual(await client.query(r'.test1.list[-1];'), 10)
            self.assertEqual(await client.query(r'.test1.col.len();'), 1)
            self.assertEqual(await client.query(r'.test2.olist[-1];'), 42)
            self.assertEqual(await client.query(r'.test2.ocol.len();'), 2)
            self.assertEqual(await client.query(r'.test3.list[-1];'), 123)
            self.assertEqual(await client.query(r'.test3.col.len();'), 3)

        client1.close()
        await client1.wait_closed()

    async def test_wrap(self, client0):
        await client0.query(r'''
            new_type('_A');  // true through mod_type
            new_type('_B');  // true through set_type
            new_type('_C', true);  // true through new_type
            new_type('_D', true);  // false through set_type
            new_type('_E', true);  // false through mod_type

            set_type('_A', {});
            set_type('_B', {}, true);
            set_type('_D', {}, false);
            set_type('_E', {});

            mod_type('_A', 'wpo', true);
            mod_type('_E', 'wpo', false);
        ''')

        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')

        await self.wait_nodes_ready(client0)

        for client in (client0, client1):
            res = await client.query('type_info("_A");')
            self.assertTrue(res['wrap_only'])

            res = await client.query('type_info("_B");')
            self.assertTrue(res['wrap_only'])

            res = await client.query('type_info("_C");')
            self.assertTrue(res['wrap_only'])

            res = await client.query('type_info("_D");')
            self.assertFalse(res['wrap_only'])

            res = await client.query('type_info("_E");')
            self.assertFalse(res['wrap_only'])

        client1.close()
        await client1.wait_closed()


if __name__ == '__main__':
    run_test(TestType())

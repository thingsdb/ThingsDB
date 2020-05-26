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

    async def test_new_type(self, client):
        await client.query(r'''
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

        await client.query(r'''
            .iris = new('User', {
                name: 'Iris',
                age: 6,
            });
            .cato = User{
                name: 'Cato',
                age: 5,
            };
            .people = new('People', {users: [.iris, .cato]});
        ''')

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

        await asyncio.sleep(1.5)

        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')

        await self.wait_nodes_ready(client0)
        iris_node0 = await client0.query('return(.iris, 2);')
        iris_node1 = await client1.query('return(.iris, 2);')

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
            return(.bricks.map(|b| b.wrap('_ColorName')), 2);
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
            return(.bricks.map(|b| b.wrap('_Color')), 2);
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

        self.assertEqual(only_name['name'], 'Iris')
        self.assertIn('#', only_name)

        iris = {k: v for k, v in only_name.items()}
        iris['age'] = 6

        await asyncio.sleep(1.5)

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
        ''')

        await client.query(r'''
            mod_type('Foo', 'del', 'foo');
        ''')

    async def test_mod(self, client):
        await client.query(r'''
            set_type('Person', {
                name: 'str',
                age: 'uint',
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
                r'`mod` or `ren` but got `x` instead'):
            await client.query(r'mod_type("Person", "x", "x");')

        # section ADD
        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `mod_type` with task `add` requires at least 4 '
                r'arguments but 3 were given'):
            await client.query(r'mod_type("Person", "add", "x");')

        with self.assertRaisesRegex(
                TypeError,
                r'function `mod_type` with task `add` expects argument 4 '
                r'to be of type `str` but got type `nil` instead'):
            await client.query(r'''
                mod_type("Person", "add", "hair_type", nil, nil);
            ''')

        with self.assertRaisesRegex(
                OperationError,
                r'function `mod_type` with task `add` requires an initial '
                r'value when adding a property to a type with one or more '
                r'instances; '
                r'1 active instance of type `Person` has been found'):
            await client.query(r'''
                mod_type("Person", "add", "hair_type", "str");
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
                r'property `age` already exists on type `Person`'):
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
                r'is being used'):
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

        # section MOD
        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `mod_type` with task `mod` takes 4 arguments '
                r'but 3 were given'):
            await client.query(r'mod_type("Person", "mod", "x");')

        with self.assertRaisesRegex(
                LookupError,
                r'type `Person` has no property `x'):
            await client.query(r'mod_type("Person", "mod", "x", "#EEEE11");')

        with self.assertRaisesRegex(
                OperationError,
                r'cannot apply type declaration `int` to `hair_type` on '
                r'type `Person`; type `Person` has 1 active instance and '
                r'the old declaration `str` is not compatible with the new '
                r'declaration'):
            await client.query(r'''
                mod_type("Person", "mod", "hair_type", "int");
            ''')

        self.assertIs(await client.query(r'''
                mod_type("Person", "mod", "hair_type", "any");
            '''), None)

        self.assertEqual(await client.query(r'.iris.hair_type;'), "blonde")

        # Section REN
        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `mod_type` with task `ren` takes 4 arguments '
                r'but 3 were given'):
            await client.query(r'mod_type("Person", "ren", "x");')

        with self.assertRaisesRegex(
                LookupError,
                r'type `Person` has no property `x'):
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
                r'property `age` already exists on type `Person`'):
            await client.query(r'''
                mod_type("Person", "ren", "hair_type", "age");
            ''')

        self.assertIs(await client.query(r'''
                mod_type("Person", "ren", "hair_type", "hair");
            '''), None)

        self.assertEqual(await client.query(r'.iris.hair;'), "blonde")

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
                r'type `Person` has no property `x`'):
            await client.query(r'mod_type("Person", "del", "x");')

        self.assertIs(await client.query(r'''
                mod_type("Person", "del", "hair");
            '''), None)

        with self.assertRaisesRegex(
                LookupError,
                r'type `Person` has no property `hair`'):
            await client.query(r'.iris.hair;')

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
                r'type `Foo` has no property `card`'):
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

    async def test_del_type(self, client):
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

        await asyncio.sleep(1.5)

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
                r'function `mod_type` with task `mod` takes 4 arguments '
                r'but 3 were given'):
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
                r'type `Tac` has no property `card`'):
            await client.query(r'''
                mod_type('Tac', 'mod', 'card', 'any');
            ''')

        with self.assertRaisesRegex(
                LookupError,
                r'property `toe` already exists on type `Tac`'):
            await client.query(r'''
                mod_type('Tac', 'add', 'toe', 'any');
            ''')

        await client.query(r'''
            del_type('Toe');
        ''')

    async def test_type_specs(self, client):
        await client.query(r'''
            set_type('_str', {test: 'str'});
            set_type('_utf8', {test: 'utf8'});
            set_type('_raw', {test: 'raw'});
            set_type('_bytes', {test: 'bytes'});
            set_type('_bool', {test: 'bool'});
            set_type('_int', {test: 'int'});
            set_type('_uint', {test: 'uint'});
            set_type('_pint', {test: 'pint'});
            set_type('_nint', {test: 'nint'});
            set_type('_float', {test: 'float'});
            set_type('_number', {test: 'number'});
            set_type('_thing', {test: 'thing'});
            set_type('_Type', {test: '_str'});
            set_type('_array', {test: '[]'});
            set_type('_set', {test: '{}'});
            set_type('_r_array', {test: '[str]'});
            set_type('_r_set', {test: '{_str}'});
            set_type('_o_str', {test: 'str?'});
            set_type('_o_array', {test: '[str?]'});
            set_type('_any', {test: 'any'});
        ''')

        with self.assertRaisesRegex(
                TypeError,
                r'mismatch in type `_str`; '
                r'type `bytes` is invalid for property `test` '
                r'with definition `str`'):
            await client.query(r'''_str{test: bytes('')};''')

        self.assertEqual(
            await client.query(r'_str{test: "x"}.test;'), 'x')
        self.assertEqual(
            await client.query(r'_str{test: "x"}.wrap("_utf8");'), {})
        self.assertEqual(
            await client.query(r'_str{test: "x"}.wrap("_raw");'),
            {'test': 'x'})
        self.assertEqual(
            await client.query(r'_str{test: "x"}.wrap("_any");'),
            {'test': 'x'})

        with self.assertRaisesRegex(
                ValueError,
                r'mismatch in type `_uint`; '
                r'property `test` only accepts integer values '
                r'greater than or equal to 0'):
            await client.query(r'''_uint{test: -1};''')

        with self.assertRaisesRegex(
                TypeError,
                r'mismatch in type `_uint`; '
                r'type `nil` is invalid for property `test` '
                r'with definition `uint`'):
            await client.query(r'''_uint{test: nil};''')

        self.assertEqual(
            await client.query(r'_uint{test: 0}.test;'), 0)
        self.assertEqual(
            await client.query(r'_uint{test: 0}.wrap("_pint");'), {})
        self.assertEqual(
            await client.query(r'_uint{test: 0}.wrap("_int");'), {'test': 0})

        with self.assertRaisesRegex(
                ValueError,
                r'mismatch in type `_pint`; '
                r'property `test` only accepts positive integer values'):
            await client.query(r'''_pint{test: 0};''')

        with self.assertRaisesRegex(
                TypeError,
                r'mismatch in type `_pint`; '
                r'type `str` is invalid for property `test` '
                r'with definition `pint`'):
            await client.query(r'''_pint{test: '0'};''')

        self.assertEqual(
            await client.query(r'_pint{test:42}.test;'), 42)
        self.assertEqual(
            await client.query(r'_pint{test:42}.wrap("_nint");'), {})
        self.assertEqual(
            await client.query(r'_pint{test:42}.wrap("_uint");'), {'test': 42})

        with self.assertRaisesRegex(
                ValueError,
                r'mismatch in type `_nint`; '
                r'property `test` only accepts negative integer values'):
            await client.query(r'''_nint{test: 0};''')

        with self.assertRaisesRegex(
                TypeError,
                r'mismatch in type `_nint`; '
                r'type `str` is invalid for property `test` '
                r'with definition `nint`'):
            await client.query(r'''_nint{test: '0'};''')

        self.assertEqual(
            await client.query(r'_nint{test:-6}.test;'), -6)
        self.assertEqual(
            await client.query(r'_nint{test:-6}.wrap("_uint");'), {})
        self.assertEqual(
            await client.query(r'_nint{test:-6}.wrap("_number")'),
            {'test': -6})


if __name__ == '__main__':
    run_test(TestType())

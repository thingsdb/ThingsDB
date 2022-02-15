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


class TestTypes(TestBase):

    title = 'Test thingsdb types'

    @default_test_setup(num_nodes=1, seed=1)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_regex(self, client):
        with self.assertRaisesRegex(
                ValueError,
                r'cannot compile regular expression \'/invalid\(regex/\', '
                r'missing closing parenthesis'):
            await client.query(r'r = /invalid(regex/;')

    async def test_comment(self, client):
        self.assertIs(await client.query(r'/* comment */'), None)
        self.assertIs(await client.query(r'// comment'), None)

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
            blob;
        ''', blob="Hi 'Iris'!!"), "Hi 'Iris'!!")
        self.assertTrue(await client.query(' ("Hello"[0] == "H") '))
        self.assertTrue(await client.query(' ("Hello"[0][-1] == "H") '))
        self.assertTrue(await client.query(' ("Hello"[-1] == "o") '))
        self.assertTrue(await client.query(' ("Hello"[-4] == "e") '))
        self.assertTrue(await client.query(' ("Hello" == "Hello") '))
        self.assertTrue(await client.query(' ("Hello" != "hello") '))
        self.assertTrue(await client.query(' ("Hello" != "Hello.") '))

    async def test_template(self, client):
        res = await client.query(r'`the answer = {6 * 7}`;')
        self.assertEqual(res, f'the answer = {6 * 7}')

        res = await client.query(r'` empty space `;')
        self.assertEqual(res, ' empty space ')

        res = await client.query(r'``;')
        self.assertEqual(res, '')

        res = await client.query(r'`   `;')
        self.assertEqual(res, '   ')

        res = await client.query(r'` {1 + 1} {2 + 2} `;')
        self.assertEqual(res, ' 2 4 ')

        await client.query(r'''
            .foo = |x| `{x} * {x} = {x * x}`;
            .bar = |x| ` ``{x}`` `;
            new_procedure('foo', .foo);
            new_procedure('bar', .bar);
        ''')
        res = await client.query(r'.foo(5);')
        self.assertEqual(res, '5 * 5 = 25')

        res = await client.query(r'.bar(5);')
        self.assertEqual(res, ' `5` ')

        res = await client.run('foo', x=7)
        self.assertEqual(res, '7 * 7 = 49')

        res = await client.run('bar', x=7)
        self.assertEqual(res, ' `7` ')

        res = await client.query(r' ```escape```')
        self.assertEqual(res, '`escape`')

        res = await client.query(r' `{{escape}}`')
        self.assertEqual(res, r'{escape}')

        with self.assertRaisesRegex(
                ZeroDivisionError,
                "division or modulo by zero"):
            await client.query(r'''
                `valid: {7*99} wrong: {613/0} never: {80*80}`;
            ''')

    async def test_instance(self, client):
        game = await client.query(r'''
            set_type('Game', {name: 'str'});
            .game = Game{name: "TicTacToe"};
        ''')
        self.assertEqual(await client.query(r'''
            Game(game_id);
        ''', game_id=game['#']), game)

        with self.assertRaisesRegex(
                TypeError,
                r'`#[0-9]+` is of type `thing`, not `Game`'):
            await client.query(r'''
                Game(.id());
            ''')

        with self.assertRaisesRegex(
                TypeError,
                r'cannot convert type `nil` to `Game`'):
            await client.query(r'''
                Game(nil);
            ''')

        with self.assertRaisesRegex(
                LookupError,
                r'collection `stuff` has no `thing` with id 12345'):
            await client.query(r'''
                Game(12345);
            ''')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'type `Game` takes at most 1 argument but 2 were given'):
            await client.query(r'''
                Game(0, 1);
            ''')

        self.assertEqual(await client.query(r'''
            Game();
        '''), {"name": ""})

    async def test_thing(self, client):
        self.assertEqual(await client.query(r'''
            [{}.id(), [{}][0].id()];
        '''), [None, None])

        self.assertEqual(await client.query(r'''
            tmp = {a: [{}], b: {}};
            [tmp.a[0].id(), tmp.b.id(), tmp.id()];
        '''), [None, None, None])

        self.assertEqual(await client.query(r'''
           {t: 0}.t;
        '''), 0)

        self.assertEqual(await client.query(r'''
           return {a: {t: 0}}, 0;
        '''), {})

        self.assertEqual(await client.query(r'''
           return {a: {t: 0}}, 1;
        '''), {'a': {}})

        self.assertEqual(await client.query(r'''
           return {a: {t: 0}}, 2;
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
        '''), [None, None])

    async def test_list(self, client):
        self.assertEqual(await client.query(r'''
            ({}.t = [1, 2, 3]).push(4);
        '''), 4)

        self.assertEqual(await client.query(r'''
            ({{}.t = [1, 2, 3]}).push(4);
        '''), 4)

        self.assertEqual(await client.query(r'''
            ({}['some key'] = [1, 2, 3]).push(4);
        '''), 4)

        self.assertEqual(await client.query(r'''
            ({{}['some key'] = [1, 2, 3]}).push(4);
        '''), 4)

        await client.query(r'''
            set_type("T", {arr: '[]'});
        ''')

        self.assertEqual(await client.query(r'''
            (T{}.arr = [1, 2, 3]).push(4);
        '''), 4)

        self.assertEqual(await client.query(r'''
            ({T{}.arr = [1, 2, 3]}).push(4);
        '''), 4)

    async def test_closure(self, client):
        with self.assertRaisesRegex(
                OverflowError,
                'integer overflow'):
            await client.query('.x = ||(1+999999999999999999999);')

        with self.assertRaisesRegex(
                OperationError,
                r'maximum recursion depth exceeded'):
            await client.query(r'''
                .a = ||.map((b = .a));
                .map(.a);
            ''')

        await client.query(r'''
            .b = ||(.x = 1);
            .a = [||.x = 1];
        ''')

        with self.assertRaisesRegex(
                OperationError,
                r'closures with side effects require a change but none is '
                r'created; use `wse\(...\)` to enforce a change;'):
            await client.query(r'''
                [1 ,2 ,3].map(.b);
            ''')

        with self.assertRaisesRegex(
                OperationError,
                r'closures with side effects require a change but none is '
                r'created; use `wse\(...\)` to enforce a change;'):
            await client.query(r'''
                [1 ,2 ,3].map(.a[0]);
            ''')

        # test two-level deep nesting
        self.assertEqual(await client.query(r'''
            .b = |k1|.map(|k2|(k1 + k2));
            .map(.b);
        '''), [["aa", "ab"], ["ba", "bb"]])

        self.assertEqual(await client.query(r"""//ti
            res = [];
            c = || {
                a = 1;
                .c = c;  /* store the closure, this will unbound the
                          * closure from the query */
                a += 1;
                res.push(a);
            };
            c();
            res;
        """), [2])

        self.assertEqual(await client.query(r'''
            res = [];
            c = |x, y, i, c| {
                a = x + i;
                b = y + i;
                i = i + 1;  // writes a variable `i` in the local scope
                if (i < 4, {
                    c(x, y, i, c);
                });
                c = a + b;
                res.push([i-1, c]);
            };
            c(7, 5, 0, c);
            res;
        '''), [[3, 18], [2, 16], [1, 14], [0, 12]])

        # Test formatting
        self.assertEqual(await client.query(r'''
            c = || {x:5};
            str(c)
        '''), r"|| {x: 5}")

    async def test_integer(self, client):
        with self.assertRaisesRegex(
                OverflowError,
                'integer overflow'):
            await client.query('9999999999999999999;')

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
        await client.query(r'''
            anna = {name: 'anne'};
            cato = {name: 'cato'};
            iris = {name: 'iris'};

            a = set(cato, iris);
            b = set(cato, anna);

            assert (a | b == set(anna, cato, iris));    // Union
            assert (a & b == set(cato));                // Intersection
            assert (a - b == set(iris));                // Difference
            assert (a ^ b == set(anna, iris));          // Symmetric difference

            assert ((set(cato, iris) | set(cato, anna))
                        == set(anna, cato, iris));      // Union move
            assert ((set(cato, iris) & set(cato, anna))
                        == set(cato));                  // Intersection inplace
            assert ((set(cato, iris) - set(cato, anna))
                        == set(iris));                  // Difference inplace
            assert ((set(cato, iris) ^ set(cato, anna))
                        == set(anna, iris));            // Symmetric difference


        ''')


if __name__ == '__main__':
    run_test(TestTypes())

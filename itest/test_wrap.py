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


class TestWrap(TestBase):

    title = 'Test wrap'

    @default_test_setup(num_nodes=2, seed=1, threshold_full_storage=10)
    async def async_run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_set_to_arr(self, client):
        await client.query(r'''
            set_type('Person', {name: 'str'});
            set_type('People', {people: '[Person]'});
            set_type('Other', {people: '[str]'});
            set_type('PS', {people: '{thing}'});
        ''')

        res = await client.query(r'''
            t = {
                people: set({name: 'Iris'})
            };
            return t.wrap('People'), 2;
        ''')
        self.assertEqual(res, {"people": [{"name": "Iris"}]})

        res = await client.query(r'''
            t = PS{
                people: set({name: 'Iris'})
            };
            return t.wrap('People'), 2;
        ''')
        self.assertEqual(res, {"people": [{"name": "Iris"}]})

        res = await client.query(r'''
            t = {
                people: set({firstname: 'Iris'})
            };
           return t.wrap('People'), 2;
        ''')
        self.assertEqual(res, {"people": [{}]})

        res = await client.query(r'''
            t = PS{
                people: set({firstname: 'Iris'})
            };
           return t.wrap('People'), 2;
        ''')
        self.assertEqual(res, {"people": [{}]})

        res = await client.query(r'''
            t = {
                people: set(Person{name: 'Iris'})
            };
            return t.wrap('Other'), 2;
        ''')
        self.assertEqual(res, {})

        res = await client.query(r'''
            t = PS{
                people: set(Person{name: 'Iris'})
            };
            return t.wrap('Other'), 2;
        ''')
        self.assertEqual(res, {})

    async def test_wrap_method(self, client):
        res = await client.query(r'''
            set_type('Math', {
                multiply: |this| this.x * this.y,
                add: |this| this.x + this.y,
            });
            set_type('XY', {
                x: 'number',
                y: 'number',
            });
            XY{x: 7, y: 6}.wrap('Math').multiply();
        ''')

        self.assertEqual(res, 42)

        with self.assertRaisesRegex(
                LookupError,
                r'type `Math` has no method `sqrt`'):
            await client.query(r'''
                XY{}.wrap("Math").sqrt();
            ''')

        res = await client.query(r'''
            XY{x: 1.5, y: 2.5}.wrap('Math').add();
        ''')

        self.assertEqual(res, 4)

    async def test_wrap(self, client):
        await client.query(r'''
            set_type(new_type('User'), {
                name: 'str',
                email: 'str',
                age: 'uint',
                address: 'str?',
                friends: '{User}',
            });
            set_type(new_type('Note'), {
                title: 'str',
                subject: 'str',
                author: 'User',
                body: 'str',
            });
            set_type(new_type('_UserName'), {
                name: 'str'
            });
            set_type(new_type('_NoteSummary'), {
                title: 'str',
                author: '_UserName'
            });
            set_type(new_type('_NotesSummary'), {
                notes: '[_NoteSummary]',
            });
        ''')

        res = await client.query(r'''
            .iris = User{
                name: 'Iris',
                email: 'iris@notreal.nl',
                age: 6,
                friends: set(),
            };
            .cato = User{
                name: 'Cato',
                email: 'cato@notreal.nl',
                age: 5,
                friends: set(),
            };

            .iris.friends.add(.cato);
            .cato.friends.add(.iris);

            .note =  Note{
                title: 'looking for the answer',
                subject: 'life the universe and everything',
                author: .iris,
                body: '42.'
            };

            .notes = set(.note);

            return .notesSummary = .wrap('_NotesSummary'), 3;
        ''')

        ids = await client.query('[.id(), .note.id(), .iris.id()];')

        self.assertEqual(res, {
            '#': ids[0],
            'notes': [{
                '#': ids[1],
                'title': 'looking for the answer',
                'author': {
                    '#': ids[2],
                    'name': 'Iris',
                },
            }]
        })

    async def test_wrap_methods(self, client):
        await client.query(r'''
            set_type('Person', {
                name: 'str',
                messages: '[str]',
                dict: 'thing?',
            });
            set_type('_Pmcount', {
                name: 'any',
                mcount: |p| p.messages.len()
            }, true);
            set_type('_Pmul', {
                name: 'any',
                mul: |a, b| a * b
            }, true);
            set_type('_Pnoargs', {
                name: 'any',
                noargs: || "hi!"
            }, true);
            set_type('_Pmore', {
                name: |p| p.name.upper(),
                other: || {other: 123},
            }, true);
            set_type('_Pwse', {
                wse: || wse(.x = 1234)
            }, true);
            set_type('_Pfut', {
                fut: || future(|| .x = 1234)
            }, true);
            set_type('_Pdeep', {
                dict: 'any',
                deep: || return {a: {b: 5}}, 2
            }, true);
        ''')

        # _Pmcount
        res = await client.query(r'''
            p = Person{
                name: 'iris',
                messages: ['hi', 'hello', 'bye']
            };
            p.wrap('_Pmcount');
        ''')

        self.assertEqual(res, {
            "name": "iris",
            "mcount": 3
        })

        # _Pmul
        res = await client.query(r'''
            p = Person{
                name: 'iris',
                messages: ['hi', 'hello', 'bye']
            };
            p.wrap('_Pmul');
        ''')

        self.assertEqual(res, {
            "name": "iris",
            "mul": '`*` not supported between `Person` and `nil`'
        })

        # _Pnoargs
        res = await client.query(r'''
            p = Person{
                name: 'iris',
                messages: ['hi', 'hello', 'bye']
            };
            p.wrap('_Pnoargs');
        ''')

        self.assertEqual(res, {
            "name": "iris",
            "noargs": "hi!"
        })

        # _Pmore
        res = await client.query(r'''
            p = Person{
                name: 'iris',
                messages: ['hi', 'hello', 'bye']
            };
            return p.wrap('_Pmore'), 2;
        ''')

        self.assertEqual(res, {
            "name": "IRIS",
            "other": {'other': 123},
        })

        # _Pwse
        res = await client.query(r'''
            p = Person{
                name: 'iris',
                messages: ['hi', 'hello', 'bye']
            };
            p.wrap('_Pwse');
        ''')

        self.assertEqual(res, {
            "wse": 'failed to compute property; method has side effects'
        })

        # _Pfut
        res = await client.query(r'''
            p = Person{
                name: 'iris',
                messages: ['hi', 'hello', 'bye']
            };
            p.wrap('_Pfut');
        ''')

        self.assertEqual(res, {
            "fut": 'failed to compute property; method contains futures'
        })

        # Pdeep
        res = await client.query(r'''
            p = Person{
                dict: {a: {b: 5}},
                name: 'iris',
                messages: ['hi', 'hello', 'bye']
            };
            p.wrap('_Pdeep');
        ''')

        self.assertEqual(res, {
            "dict": {},
            "deep": {"a": {"b": 5}}
        })

    async def test_skip_nil(self, client):
        with self.assertRaisesRegex(
                TypeError,
                r'invalid declaration for `x` on type `T`; '
                r'skip nil flags for property which cannot contain nil;'):
            await client.query("""//ti
                set_type('T', {x: '?int'})
            """)

        await client.query("""//ti
            set_type('T', {
                name: 'str',
                age: 'int?',
                other: '[str]?',
            });
            set_type('_T', {
                name: 'str',
                age: '?int?',
                other: '?any',
            }, true, true);
        """)
        res = await client.query("""//ti
            .orig = [
                T{name: 'a'},
                T{name: 'b', age: 123, other: ['other']},
                {name: 'c', age: nil, other: nil}
            ].map_wrap('_T');
        """)
        self.assertEqual(res, [
            {'name': 'a'},
            {'name': 'b', 'age': 123, 'other': ['other']},
            {'name': 'c'}
        ])
        res = await client.query("""//ti
            .orig.copy();
        """)
        self.assertEqual(res, [
            {'name': 'a'},
            {'name': 'b', 'age': 123, 'other': ['other']},
            {'name': 'c'}
        ])

    async def test_skip_false(self, client):
        with self.assertRaisesRegex(
                ValueError,
                r'invalid declaration for `x` on type `T`; duplicate flags;'):
            await client.query("""//ti
                set_type('T', {x: '!!int'})
            """)

        await client.query("""//ti
            set_type('T', {
                name: 'str',
                age: 'int?',
                other: '[str]?',
            });
            set_type('_T', {
                name: 'str',
                age: '!int?',
                other: '!any',
            }, true, true);
        """)
        res = await client.query("""//ti
            .orig = [
                T{name: 'a'},
                T{name: 'b', age: 123, other: ['other']},
                {name: 'c', age: nil, other: []},
                {name: 'd', age: 0, other: nil}
            ].map_wrap('_T');
        """)
        self.assertEqual(res, [
            {'name': 'a'},
            {'name': 'b', 'age': 123, 'other': ['other']},
            {'name': 'c'},
            {'name': 'd'}
        ])
        res = await client.query("""//ti
            .orig.copy();
        """)
        self.assertEqual(res, [
            {'name': 'a'},
            {'name': 'b', 'age': 123, 'other': ['other']},
            {'name': 'c'},
            {'name': 'd'}
        ])

if __name__ == '__main__':
    run_test(TestWrap())

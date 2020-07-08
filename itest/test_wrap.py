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
    async def run(self):

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
        ''')

        res = await client.query(r'''
            t = {
                people: set({name: 'Iris'})
            };
            return(t.wrap('People'), 2);
        ''')
        self.assertEqual(res, {"people": [{"name": "Iris"}]})

        res = await client.query(r'''
            t = {
                people: set({firstname: 'Iris'})
            };
           return(t.wrap('People'), 2);
        ''')
        self.assertEqual(res, {"people": [{}]})

        res = await client.query(r'''
            t = {
                people: set(Person{name: 'Iris'})
            };
            return(t.wrap('Other'), 2);
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

        with self.assertRaisesRegex(
                LookupError,
                r'type `Math` has no method `id`'):
            await client.query(r'''
                XY{}.wrap("Math").id();
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

            return(.notesSummary = .wrap('_NotesSummary'), 3);
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


if __name__ == '__main__':
    run_test(TestWrap())

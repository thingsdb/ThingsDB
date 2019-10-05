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
        client.use('stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_wrap(self, client):
        await client.query(r'''
            new_type('User', {
                name: 'str',
                email: 'str',
                age: 'uint',
                address: 'str?',
                friends: '{User}',
            });
            new_type('Note', {
                title: 'str',
                subject: 'str',
                author: 'User',
                body: 'str',
            });
            new_type('W_UserName', {
                name: 'str'
            });
            new_type('W_NoteSummary', {
                title: 'str',
                author: 'W_UserName'
            });
            new_type('W_NotesSummary', {
                notes: '[W_NoteSummary]',
            });
        ''')

        res = await client.query(r'''
            .iris = User({
                name: 'Iris',
                email: 'iris@notreal.nl',
                age: 6,
                friends: set(),
            });
            .cato = User({
                name: 'Cato',
                email: 'cato@notreal.nl',
                age: 5,
                friends: set(),
            });

            .iris.friends.add(.cato);
            .cato.friends.add(.iris);

            .note =  Note({
                title: 'looking for the answer',
                subject: 'life the universe and everything',
                author: .iris,
                body: '42.'
            });

            .notes = set(.note);

            return(.notesSummary = .wrap('W_NotesSummary'), 3);
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

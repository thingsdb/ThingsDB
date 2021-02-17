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


class TestRelations(TestBase):

    title = 'Test relations between types'

    def with_node1(self):
        if hasattr(self, 'node1'):
            return True
        print('''
            WARNING: Test requires a second node!!!
        ''')

    @default_test_setup(num_nodes=2, seed=1, threshold_full_storage=10)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        # add another node otherwise backups are not possible
        if hasattr(self, 'node1'):
            await self.node1.join_until_ready(client)

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_errors(self, client):
        await client.query(r'''
            new_type('A');
            new_type('B');
            new_type('C');

            set_type('A', {
                bstrict: 'B',
                a: 'A?',
                b: 'B?',
                c: 'C?',
                t: 'thing',
            });

            set_type('B', {
                a: 'A?',
            });

            set_type('C', {
                astrict: 'A',
                a: 'A?',
                t: 'thing',
            });
        ''')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `mod_type` with task `rel` takes 4 arguments '
                r'but 3 were given'):
            await client.query(r'''
                mod_type('A', 'rel', 'a');
            ''')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `mod_type` with task `rel` takes 4 arguments '
                r'but 5 were given'):
            await client.query(r'''
                mod_type('A', 'rel', 'a', nil, nil);
            ''')

        with self.assertRaisesRegex(
                TypeError,
                r'function `mod_type` expects argument 3 to be of '
                r'type `str` but got type `closure` instead'):
            await client.query(r'''
                mod_type('A', 'rel', ||nil, nil);
            ''')

        with self.assertRaisesRegex(
                TypeError,
                r'function `mod_type` with task `rel` expects argument 4 to '
                r'be of type `str` or type `nil` but '
                r'got type `thing` instead'):
            await client.query(r'''
                mod_type('A', 'rel', 'a', {});
            ''')

        with self.assertRaisesRegex(
                LookupError,
                r'type `A` has no property `x`'):
            await client.query(r'''
                mod_type('A', 'rel', 'x', 'b');
            ''')

        with self.assertRaisesRegex(
                TypeError,
                r'cannot create relation; property `c` on type `A` is '
                r'referring to type `C`'):
            await client.query(r'''
                mod_type('A', 'rel', 'a', 'c');
            ''')

        with self.assertRaisesRegex(
                TypeError,
                r'relations may only be configured between restricted '
                r'sets and/or nillable types'):
            await client.query(r'''
                mod_type('A', 'rel', 'c', 'astrict');
            ''')

        with self.assertRaisesRegex(
                TypeError,
                r'relations may only be configured between restricted '
                r'sets and/or nillable types'):
            await client.query(r'''
                mod_type('A', 'rel', 'bstrict', 'a');
            ''')

    async def test_mod_type(self, client):
        await client.query(r'''
            new_type('User');
            new_type('Space');

            set_type('User', {
                space: 'Space?'
            });

            set_type('Space', {
                user: 'User?'
            });

            mod_type('User', 'rel', 'space', 'user');
        ''')

        self.assertTrue(await client.query(r'''
            u = User{};
            u.space = Space{};
            u.space.user == u;
        '''))

        self.assertEqual(await client.query(r'''
            u1 = User{};
            s1 = Space{};
            u1.space = s1;
            assert (s1.user == u1);
            assert (u1.space == s1);

            u2 = User{};
            s2 = Space{};
            s2.user = u2;
            assert (s2.user == u2);
            assert (u2.space == s2);

            u = User{};
            s = Space{};
            u.space = s;
            assert (s.user == u);
            assert (u.space == s);

            u.space = s2;
            assert (s.user == nil);
            assert (u.space == s2);
            assert (s2.user == u);
            assert (u2.space == nil);

            'OK';
        '''), 'OK')

    async def test_mod_type_multi_node(self, client0):
        if not self.with_node1():
            return
        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')

        await client0.query(r'''
            new_type('User');
            new_type('Space');
            new_type('Self');

            set_type('Self', {
                self: 'Self?'
            });

            set_type('User', {
                space: 'Space?'
            });

            set_type('Space', {
                user: 'User?'
            });

            mod_type('User', 'rel', 'space', 'user');
        ''')

        await client0.query(r'''
            .u1 = User{};
            .s1 = Space{};
            .u1.space = .s1;
        ''')

        for client in (client0, client1):
            self.assertEqual(await client.query(r'''
                assert (.s1.user == .u1);
                assert (.u1.space == .s1);
                'OK';
            '''), 'OK')


if __name__ == '__main__':
    run_test(TestRelations())

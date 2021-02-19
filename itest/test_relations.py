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

    @default_test_setup(num_nodes=1, seed=1, threshold_full_storage=10)
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
                r'failed to create relation; property `c` on type `A` is '
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

    async def test_type_state_error(self, client):
        await client.query(r'''
            new_type('A');
            new_type('B');

            set_type('A', {
                b: 'B?'
            });

            set_type('B', {
                a: 'A?'
            });
        ''')

        with self.assertRaisesRegex(
                TypeError,
                r'failed to create relation; '
                r'property `b` on a `thing` is referring to a second '
                r'`thing` while property `a` on that second thing is '
                r'referring to a third `thing`'):
            await client.query(r'''
                a1 = A{};
                a2 = A{};
                b1 = B{};

                a1.b = b1;
                b1.a = a2;

                mod_type('A', 'rel', 'b', 'a');
            ''')

        with self.assertRaisesRegex(
                TypeError,
                r'failed to create relation; '
                r'property `b` on `#\d+` is referring to `#\d+` while '
                r'property `a` on `#\d+` is referring to `#\d+`'):
            await client.query(r'''
                .a1 = A{};
                .a2 = A{};
                .b1 = B{};

                .a1.b = .b1;
                .b1.a = .a2;

                mod_type('A', 'rel', 'b', 'a');
            ''')

    async def test_set_state_error_1(self, client):
        await client.query(r'''
            new_type('A');
            new_type('B');

            set_type('A', {
                b: 'B?'
            });

            set_type('B', {
                aa: '{A}'
            });
        ''')

        with self.assertRaisesRegex(
                TypeError,
                r'failed to create relation; at least one thing belongs to '
                r'at least two different sets; \(property `aa` on type `B`'):
            await client.query(r'''
                a1 = A{};
                b1 = B{};
                b2 = B{};

                b1.aa.add(a1);
                b2.aa.add(a1);

                mod_type('A', 'rel', 'b', 'aa');
            ''')

        with self.assertRaisesRegex(
                TypeError,
                r'failed to create relation; thing `#\d+` belongs to at '
                r'least two different sets; \(property `aa` on type `B`\)'):
            await client.query(r'''
                .a1 = A{};
                .b1 = B{};
                .b2 = B{};

                .b1.aa.add(.a1);
                .b2.aa.add(.a1);

                mod_type('A', 'rel', 'b', 'aa');
            ''')

    async def test_set_state_error_2(self, client):
        await client.query(r'''
            new_type('A');
            new_type('B');

            set_type('A', {
                b: 'B?'
            });

            set_type('B', {
                aa: '{A}'
            });
        ''')

        with self.assertRaisesRegex(
                TypeError,
                r'failed to create relation; at least one thing belongs '
                r'to a set \(`B.aa`\) of a different thing than the thing '
                r'it is referring to \(`A.b`\)'):
            await client.query(r'''
                a1 = A{};
                b1 = B{};
                b2 = B{};

                a1.b = b1;
                b2.aa.add(a1);

                mod_type('A', 'rel', 'b', 'aa');
            ''')

        with self.assertRaisesRegex(
                TypeError,
                r'failed to create relation; thing `#\d+` belongs to a '
                r'set `aa` on `#\d+` but is referring to `#\d+`'):
            await client.query(r'''
                .a1 = A{};
                .b1 = B{};
                .b2 = B{};

                .a1.b = .b1;
                .b2.aa.add(.a1);

                mod_type('A', 'rel', 'b', 'aa');
            ''')

    async def test_set_set_init_state(self, client0):
        if not self.with_node1():
            return
        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')

        await client0.query(r'''
            new_type('A');
            new_type('B');
            new_type('C');

            set_type('A', {
                bb: '{B}'
            });

            set_type('B', {
                aa: '{A}'
            });

            set_type('C', {
                cc: '{C}'
            });
        ''')

        self.assertEqual(await client0.query(r'''
            .a1 = A{};
            .a2 = A{};
            .b1 = B{};
            .b2 = B{};
            .c1 = C{};
            .c2 = C{};

            .a1.bb.add(.b1, .b2);
            .b1.aa.add(.a1);
            .b2.aa.add(.a2);
            .c1.cc.add(.c1);
            .c2.cc.add(.c1);

            a1 = A{};
            a2 = A{};
            b1 = B{};
            b2 = B{};
            c1 = C{};
            c2 = C{};

            a1.bb.add(b1, b2);
            b1.aa.add(a1);
            b2.aa.add(a2);
            c1.cc.add(c1);
            c2.cc.add(c1);

            mod_type('A', 'rel', 'bb', 'aa');
            mod_type('C', 'rel', 'cc', 'cc');

            assert(a1.bb.has(b1));
            assert(a1.bb.has(b2));

            assert(!a2.bb.has(b1));
            assert(a2.bb.has(b2));

            assert(b1.aa.has(a1));
            assert(!b1.aa.has(a2));

            assert(b2.aa.has(a1));
            assert(b2.aa.has(a2));

            'OK';
        '''), 'OK')

        await asyncio.sleep(0.2)

        for client in (client0, client1):
            self.assertEqual(await client.query(r'''
                assert(.a1.bb.has(.b1));
                assert(.a1.bb.has(.b2));

                assert(!.a2.bb.has(.b1));
                assert(.a2.bb.has(.b2));

                assert(.b1.aa.has(.a1));
                assert(!.b1.aa.has(.a2));

                assert(.b2.aa.has(.a1));
                assert(.b2.aa.has(.a2));

                'OK';
            '''), 'OK')

    async def test_type_type_init_state(self, client0):
        if not self.with_node1():
            return
        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')

        await client0.query(r'''
            new_type('A');
            new_type('B');
            new_type('C');

            set_type('A', {
                b: 'B?'
            });

            set_type('B', {
                a: 'A?'
            });

            set_type('C', {
                c: 'C?'
            });
        ''')

        self.assertEqual(await client0.query(r'''
            .a1 = A{};
            .a2 = A{};
            .a3 = A{};
            .b1 = B{};
            .b2 = B{};
            .c1 = C{};
            .c2 = C{};

            .a1.b = .b1;
            .a2.b = .b2;
            .b2.a = .a2;
            .c1.c = .c1;

            a1 = A{};
            a2 = A{};
            a3 = A{};
            b1 = B{};
            b2 = B{};
            c1 = C{};
            c2 = C{};

            a1.b = b1;
            a2.b = b2;
            b2.a = a2;
            c1.c = c1;

            mod_type('A', 'rel', 'b', 'a');
            mod_type('C', 'rel', 'c', 'c');

            assert(a1.b == b1);
            assert(a2.b == b2);
            assert(a3.b == nil);
            assert(b1.a == a1);
            assert(b2.a == a2);
            assert(c1.c == c1);
            assert(c2.c == nil);

            'OK';
        '''), 'OK')

        await asyncio.sleep(0.2)

        for client in (client0, client1):
            self.assertEqual(await client.query(r'''
                assert(.a1.b == .b1);
                assert(.a2.b == .b2);
                assert(.a3.b == nil);
                assert(.b1.a == .a1);
                assert(.b2.a == .a2);
                assert(.c1.c == .c1);
                assert(.c2.c == nil);

                'OK';
            '''), 'OK')

    async def test_type_set_init_state(self, client0):
        if not self.with_node1():
            return
        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')

        await client0.query(r'''
            new_type('A');
            new_type('B');
            new_type('C');

            set_type('A', {
                b: 'B?'
            });

            set_type('B', {
                aa: '{A}'
            });

            set_type('C', {
                c: 'C?',
                cc: '{C},
            });
        ''')

        self.assertEqual(await client0.query(r'''
            .a1 = A{};
            .a2 = A{};
            .a3 = A{};
            .a4 = A{};
            .b1 = B{};
            .b2 = B{};
            .c1 = C{};
            .c2 = C{};
            .c3 = C{};
            .c4 = C{};
            .c5 = C{};

            .a1.b = .b1;
            .a2.b = .b1;
            .b1.aa.add(.a2);
            .b2.aa.add(.a3);
            .c1.c = .c1;
            .c2.c = .c1;
            .c1.cc.add(.c2);
            .c1.cc.add(.c3);
            .c4.cc.add(.c5);

            a1 = A{};
            a2 = A{};
            a3 = A{};
            a4 = A{};
            b1 = B{};
            b2 = B{};
            c1 = C{};
            c2 = C{};
            c3 = C{};
            c4 = C{};
            c5 = C{};

            a1.b = b1;
            a2.b = b1;
            b1.aa.add(a2);
            b2.aa.add(a3);
            c1.c = c1;
            c2.c = c1;
            c1.cc.add(c2);
            c1.cc.add(c3);
            c4.cc.add(c5);

            mod_type('A', 'rel', 'b', 'aa');
            mod_type('C', 'rel', 'c', 'cc');

            assert(a1.b == b1);
            assert(a2.b == b1);
            assert(a3.b == b2);
            assert(a4.b == nil);
            assert(c1.c == c1);
            assert(c2.c == c1);
            assert(c3.c == c1);
            assert(c4.c == nil);
            assert(c5.c == c4);

            assert(b1.aa.has(a1));
            assert(b1.aa.has(a2));
            assert(b1.aa.len() == 2);

            'OK';
        '''), 'OK')

        await asyncio.sleep(0.2)

        for client in (client0, client1):
            self.assertEqual(await client.query(r'''
                assert(.a1.b == .b1);
                assert(.a2.b == .b2);
                assert(.a3.b == nil);
                assert(.b1.a == .a1);
                assert(.b2.a == .a2);

                'OK';
            '''), 'OK')

    async def test_type_to_type(self, client):
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

    async def test_type_to_self(self, client):
        await client.query(r'''
            new_type('Self');
            set_type('Self', {
                self: 'Self?'
            });

            mod_type('Self', 'rel', 'self', 'self');
        ''')

        self.assertEqual(await client.query(r'''
            s1 = Self{};
            s2 = Self{};
            s1.self = s2;
            assert (s1.self == s2);
            assert (s2.self == s1);

            // a second time shoudl work as well
            s1.self = s2;
            assert (s1.self == s2);
            assert (s2.self == s1);

            // remove assignment
            s1.self = nil;
            assert(is_nil(s1.self));
            assert(is_nil(s2.self));

            // Restore assigmment
            s1.self = s2;
            assert (s1.self == s2);
            assert (s2.self == s1);

            // Remove relation
            mod_type('Self', 'rel', 'self', nil);

            // Check is relation is removed
            s1.self = nil;
            assert(is_nil(s1.self));
            assert(s2.self == s1);

            'OK';
        '''), 'OK')

        self.assertEqual(await client.query(r'''
            s1 = Self{};
            s1.self = s1;
            assert (s1.self == s1);

            // a second time should work as well
            s1.self = s1;
            assert (s1.self == s1);

            // remove assignment
            s1.self = nil;
            assert(is_nil(s1.self));

            'OK';
        '''), 'OK')

    async def test_type_to_set(self, client):
        await client.query(r'''
            new_type('A');
            new_type('B');
            new_type('C');

            set_type('A', {
                b: '{B}'
            });

            set_type('B', {
                a: 'A?'
            });

            set_type('C', {
                c: 'C?',
                cc: '{C}'
            });

            mod_type('A', 'rel', 'b', 'a');
            mod_type('C', 'rel', 'c', 'cc');
        ''')

        self.assertEqual(await client.query(r'''
            a1 = A{};
            a2 = A{};
            b1 = B{};
            b2 = B{};

            b1.a = a1;
            b2.a = a1;

            assert(a1.b.has(b1));
            assert(a1.b.has(b2));
            assert(!a2.b.has(b1));
            assert(!a2.b.has(b2));

            b2.a = a2;

            assert(a1.b.has(b1));
            assert(!a1.b.has(b2));
            assert(!a2.b.has(b1));
            assert(a2.b.has(b2));

            a2.b.add(b1);

            assert(!a1.b.has(b1));
            assert(!a1.b.has(b2));
            assert(a2.b.has(b1));
            assert(a2.b.has(b2));

            'OK';
        '''), 'OK')

    async def test_set_to_set(self, client):
        await client.query(r'''
            new_type('A');
            new_type('B');
            new_type('C');

            set_type('A', {
                b: '{B}'
            });

            set_type('B', {
                a: '{A}'
            });

            set_type('C', {
                c: '{C}'
            });

            mod_type('A', 'rel', 'b', 'a');
            mod_type('C', 'rel', 'c', 'c');
        ''')

        self.assertEqual(await client.query(r'''
            a = A{};
            b = B{};
            b.a.add(a);

            assert(a.b.has(b));
            assert(b.a.has(a));

            b.a.remove(a);

            assert(!a.b.has(b));
            assert(!b.a.has(a));

            'OK';
        '''), 'OK')

        self.assertEqual(await client.query(r'''
            c = C{};
            cc = C{};
            c.c.add(c);
            c.c.add(cc);

            assert(c.c.has(c));
            assert(c.c.has(cc));
            assert(cc.c.has(c));
            assert(!cc.c.has(cc));

            c.c.remove(||true);

            assert(!c.c.has(c));
            assert(!c.c.has(cc));
            assert(!cc.c.has(c));
            assert(!cc.c.has(cc));

            'OK';
        '''), 'OK')

    async def test_set_to_set_multi_node(self, client0):
        if not self.with_node1():
            return
        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')

        await client0.query(r'''
            new_type('A');
            new_type('B');
            new_type('C');

            set_type('A', {
                b: '{B}'
            });

            set_type('B', {
                a: '{A}'
            });

            set_type('C', {
                c: '{C}'
            });

            mod_type('A', 'rel', 'b', 'a');
            mod_type('C', 'rel', 'c', 'c');
        ''')

        self.assertEqual(await client0.query(r'''
            .a = A{};
            .b = B{};
            .b.a.add(.a);

            'OK';
        '''), 'OK')

        await asyncio.sleep(0.2)

        for client in (client0, client1):
            self.assertEqual(await client.query(r'''
                assert(.a.b.has(.b));
                assert(.b.a.has(.a));

                'OK';
            '''), 'OK')

        self.assertEqual(await client0.query(r'''
            .b.a.remove(.a);

            'OK';
        '''), 'OK')

        await asyncio.sleep(0.2)

        for client in (client0, client1):
            self.assertEqual(await client.query(r'''
                assert(!.a.b.has(.b));
                assert(!.b.a.has(.a));

                'OK';
            '''), 'OK')

        self.assertEqual(await client0.query(r'''
            .c = C{};
            .cc = C{};
            .c.c.add(.c);
            .c.c.add(.cc);

            'OK';
        '''), 'OK')

        await asyncio.sleep(0.2)

        for client in (client0, client1):
            self.assertEqual(await client.query(r'''
                assert(.c.c.has(.c));
                assert(.c.c.has(.cc));
                assert(.cc.c.has(.c));
                assert(!.cc.c.has(.cc));
                'OK';
            '''), 'OK')

        self.assertEqual(await client0.query(r'''
            .c.c.remove(||true);
            'OK';
        '''), 'OK')

        await asyncio.sleep(0.2)

        for client in (client0, client1):
            self.assertEqual(await client.query(r'''
                assert(!.c.c.has(.c));
                assert(!.c.c.has(.cc));
                assert(!.cc.c.has(.c));
                assert(!.cc.c.has(.cc));

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

        await asyncio.sleep(0.2)

        for client in (client0, client1):
            self.assertEqual(await client.query(r'''
                assert (.s1.user == .u1);
                assert (.u1.space == .s1);
                'OK';
            '''), 'OK')

        await client0.query(r'''
            .s1.user = nil;
        ''')

        await asyncio.sleep(0.2)

        for client in (client0, client1):
            self.assertEqual(await client.query(r'''
                assert (.s1.user == nil);
                assert (.u1.space == nil);
                'OK';
            '''), 'OK')

        await client0.query(r'''
            .u1.space = .s1;
        ''')

        await asyncio.sleep(0.2)

        for client in (client0, client1):
            self.assertEqual(await client.query(r'''
                assert (.s1.user == .u1);
                assert (.u1.space == .s1);
                'OK';
            '''), 'OK')

        await client0.query(r'''
            mod_type('User', 'rel', 'space', nil);
            .s1.user = nil;
        ''')

        await asyncio.sleep(0.2)

        for client in (client0, client1):
            self.assertEqual(await client.query(r'''
                assert (.s1.user == nil);
                assert (.u1.space == .s1);
                'OK';
            '''), 'OK')


if __name__ == '__main__':
    run_test(TestRelations())

#!/usr/bin/env python
import asyncio
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client
from thingsdb.exceptions import TypeError
from thingsdb.exceptions import NumArgumentsError
from thingsdb.exceptions import LookupError
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
    async def async_run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        # add another node
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
            new_type('D');
            new_type('E');
            new_type('F');

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

            set_type('D', {
                e: 'E?'
            });

            set_type('E', {
                dd: '{D}'
            });

            set_type('F', {
                f: 'F?',
                ff: 'F?',
                fff: '{F}'
            });

            mod_type('D', 'rel', 'e', 'dd');
            mod_type('F', 'rel', 'f', 'f');
            mod_type('F', 'rel', 'fff', 'fff');
        ''')

        with self.assertRaisesRegex(
                TypeError,
                r'relation for `f` on type `F` already exist'):
            await client.query(r'''
                mod_type('F', 'rel', 'ff', 'f');
            ''')

        with self.assertRaisesRegex(
                TypeError,
                r'cannot delete a property with a relation; '
                r'you might want to remove the relation by using: '
                r'`mod_type\("D", "rel", "e", nil\);`'):
            await client.query(r'''
                mod_type('D', 'del', 'e');
            ''')

        with self.assertRaisesRegex(
                TypeError,
                r'cannot modify a property with a relation; '
                r'you might want to remove the relation by using: '
                r'`mod_type\("D", "rel", "e", nil\);`'):
            await client.query(r'''
                mod_type('D', 'mod', 'e', 'E');
            ''')

        res = await client.query(r'''
            .f = F{};
            f2 = F{};
            f3 = F{};
            .f.fff.add(.f);
            .f.fff.add(f2);
            mod_type('F', 'ren', 'fff', 'others');
            .f.others.add(f3);
            [.f.others.len(), f2.others.len(), f3.others.len()];
        ''')
        self.assertEqual(res, [3, 1, 1])

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
                r'sets and/or nillable typed'):
            await client.query(r'''
                mod_type('A', 'rel', 'c', 'astrict');
            ''')

        with self.assertRaisesRegex(
                TypeError,
                r'relations may only be configured between restricted '
                r'sets and/or nillable typed'):
            await client.query(r'''
                mod_type('A', 'rel', 'bstrict', 'a');
            ''')

    async def test_type_state_error(self, client):
        await client.query(r'''
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
                cx: 'C?',
                cy: 'C?'
            });
        ''')

        with self.assertRaisesRegex(
                TypeError,
                r'failed to create relation; '
                r'relations must be created using a property on a stored '
                r'thing \(a thing with an Id\)'):
            await client.query(r'''
                c1 = C{};
                c2 = C{};
                c1.cx = c1;
                c1.cy = c2;

                mod_type('C', 'rel', 'cx', 'cy');
            ''')

        with self.assertRaisesRegex(
                TypeError,
                r'failed to create relation; '
                r'property `cy` on `#\d+` is referring to `#\d+` '
                r'while property `cx` on `#\d+` is '
                r'referring to `#\d+`'):
            await client.query(r'''
                .c1 = C{};
                .c2 = C{};
                .c1.cx = .c2;
                .c1.cy = .c1;

                mod_type('C', 'rel', 'cx', 'cy');
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
                r'failed to create relation; relations must be created using '
                r'a property on a stored thing \(a thing with an Id\)'):
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

            mod_type('A', 'rel', 'bb', 'aa');
            mod_type('C', 'rel', 'cc', 'cc');

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

        client = client1

        self.assertEqual(await client0.query(r'''
            new_type('D');
            set_type('D', {
                da: '{D}',
                db: '{D}'
            });

            mod_type('D', 'rel', 'da', 'db');

            'OK';
        '''), 'OK')

        await client.query(r'''wse();''')
        res = await client.query(r'''export();''')
        self.assertEqual(res, r'''
try(commit('Source: collection `stuff`'));

new_type('A');
new_type('B');
new_type('C');
new_type('D');


set_type('A', {
  bb: '{B}',
});
set_type('B', {
  aa: '{A}',
});
set_type('C', {
  cc: '{C}',
});
set_type('D', {
  da: '{D}',
  db: '{D}',
});

mod_type('A', 'rel', 'bb', 'aa');
mod_type('C', 'rel', 'cc', 'cc');
mod_type('D', 'rel', 'da', 'db');




'DONE';
'''.lstrip().replace('  ', '\t'))

    async def test_type_type_init_state(self, client0):
        if not self.with_node1():
            return
        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')

        await client0.query(r'''
            new_type('A');
            new_type('B');
            new_type('C');
            new_type('D');

            set_type('A', {
                b: 'B?'
            });

            set_type('B', {
                a: 'A?'
            });

            set_type('C', {
                c: 'C?'
            });

            set_type('D', {
                dx: 'D?',
                dy: 'D?'
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
            .d1 = D{};
            .d2 = D{};
            .d3 = D{};

            .a1.b = .b1;
            .a2.b = .b2;
            .b2.a = .a2;
            .c1.c = .c1;
            .d1.dx = .d2;
            .d3.dx = .d3;

            mod_type('A', 'rel', 'b', 'a');
            mod_type('C', 'rel', 'c', 'c');
            mod_type('D', 'rel', 'dx', 'dy');

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
                assert(.d1.dx == .d2);
                assert(.d1.dy == nil);
                assert(.d2.dx == nil);
                assert(.d2.dy == .d1);
                assert(.d3.dx == .d3);
                assert(.d3.dy == .d3);

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
                cc: '{C}',
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

            mod_type('A', 'rel', 'b', 'aa');
            mod_type('C', 'rel', 'c', 'cc');

            assert(types_info().len(), 3);

            'OK';
        '''), 'OK')

        await asyncio.sleep(0.2)

        for client in (client0, client1):
            self.assertEqual(await client.query(r'''
                assert(.a1.b == .b1);
                assert(.a2.b == .b1);
                assert(.a3.b == .b2);
                assert(.a4.b == nil);
                assert(.c1.c == .c1);
                assert(.c2.c == .c1);
                assert(.c3.c == .c1);
                assert(.c4.c == nil);
                assert(.c5.c == .c4);

                assert(.b1.aa.has(.a1));
                assert(.b1.aa.has(.a2));
                assert(.b1.aa.len() == 2);
                assert(.b2.aa.has(.a3));
                assert(.b2.aa.len() == 1);

                assert(.c1.cc.has(.c1));
                assert(.c1.cc.has(.c2));
                assert(.c1.cc.has(.c3));
                assert(.c1.cc.len() == 3);
                assert(.c2.cc.len() == 0);
                assert(.c3.cc.len() == 0);
                assert(.c4.cc.has(.c5));
                assert(.c4.cc.len() == 1);
                assert(.c5.cc.len() == 0);

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
            .u = User{};
            .u.space = Space{};
            .u.space.user == .u;
        '''))

        self.assertEqual(await client.query(r'''
            .u1 = User{};
            s1 = Space{};
            .u1.space = s1;
            assert (s1.user == .u1);
            assert (.u1.space == s1);

            u2 = User{};
            .s2 = Space{};
            .s2.user = u2;
            assert (.s2.user == u2);
            assert (u2.space == .s2);

            .u = User{};
            s = Space{};
            .u.space = s;
            assert (s.user == .u);
            assert (.u.space == s);

            .u.space = .s2;
            assert (s.user == nil);
            assert (.u.space == .s2);
            assert (.s2.user == .u);
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
            .s1 = Self{};
            s2 = Self{};
            .s1.self = s2;
            assert (.s1.self == s2);
            assert (s2.self == .s1);

            // a second time shoudl work as well
            .s1.self = s2;
            assert (.s1.self == s2);
            assert (s2.self == .s1);

            // remove assignment
            .s1.self = nil;
            assert(is_nil(.s1.self));
            assert(is_nil(s2.self));

            // Restore assigmment
            .s1.self = s2;
            assert (.s1.self == s2);
            assert (s2.self == .s1);

            // Remove relation
            mod_type('Self', 'rel', 'self', nil);

            // Check is relation is removed
            .s1.self = nil;
            assert(is_nil(.s1.self));
            assert(s2.self == .s1);

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
            .b1 = B{};
            .b2 = B{};

            .b1.a = a1;
            .b2.a = a1;

            assert(a1.b.has(.b1));
            assert(a1.b.has(.b2));
            assert(!a2.b.has(.b1));
            assert(!a2.b.has(.b2));

            .b2.a = a2;

            assert(a1.b.has(.b1));
            assert(!a1.b.has(.b2));
            assert(!a2.b.has(.b1));
            assert(a2.b.has(.b2));

            a2.b.add(.b1);

            assert(!a1.b.has(.b1));
            assert(!a1.b.has(.b2));
            assert(a2.b.has(.b1));
            assert(a2.b.has(.b2));

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
            .b = B{};
            .b.a.add(a);

            assert(a.b.has(.b));
            assert(.b.a.has(a));

            .b.a.remove(a);

            assert(!a.b.has(.b));
            assert(!.b.a.has(a));

            'OK';
        '''), 'OK')

        self.assertEqual(await client.query(r'''
            .c = c = C{};
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

    async def test_multi_node_replace_val(self, client0):
        if not self.with_node1():
            return
        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')

        await client0.query(r'''
            new_type('A');
            new_type('B');

            set_type('A', {
                b: 'B?'
            });

            set_type('B', {
                aa: '{A}'
            });

            mod_type('A', 'rel', 'b', 'aa');
        ''')

        await client0.query(r'''
            .a1 = A{};
            .a2 = A{};
            .a3 = A{};
            .a4 = A{};

            .b1 = B{};
            .b2 = B{};

            .a1.b = .b1;
            .b1.aa = set(.a2, .a3);
            .b2.aa |= set(.a3, .a4);
            .a4.b = nil;
        ''')

        await asyncio.sleep(0.2)

        for client in (client0, client1):
            self.assertEqual(await client.query(r'''
                assert (.a1.b == nil);
                assert (.a2.b == .b1);
                assert (.a3.b == .b2);
                assert (.a4.b == nil);
                assert (.b2.aa.has(.a3));
                assert (.b1.aa.len() == 1);
                assert (.b2.aa.has(.a3));
                assert (.b2.aa.len() == 1);

                'OK';
            '''), 'OK')

    async def test_set_operations(self, client0):
        if not self.with_node1():
            return
        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')

        await client0.query(r'''
            new_type('A');
            new_type('B');

            set_type('A', {
                a: 'A?',
                aa: '{A}',
                aaa: '{A}',
                bb: '{B}',
            });

            set_type('B', {
                aa: '{A}'
            });

            mod_type('A', 'rel', 'a', 'aa');
            mod_type('A', 'rel', 'bb', 'aa');
            mod_type('A', 'rel', 'aaa', 'aaa');
        ''')

        self.assertEqual(await client0.query(r'''
            .a1 = A{};
            .a2 = A{};
            .a3 = A{};
            .a4 = A{};

            .b1 = B{};
            .b2 = B{};

            .b1.aa.add(.a1, .a2, .a3);

            assert (.a1.bb == set(.b1));
            assert (.a2.bb == set(.b1));
            assert (.a3.bb == set(.b1));

            .b1.aa = set(.a2, .a3);

            assert (.a1.bb == set());
            assert (.a2.bb == set(.b1));
            assert (.a3.bb == set(.b1));

            .b1.aa |= set(.a3, .a4);

            assert (.a1.bb == set());
            assert (.a2.bb == set(.b1));
            assert (.a3.bb == set(.b1));
            assert (.a3.bb == set(.b1));

            .b1.aa ^= set(.a1, .a3, .a4);
            assert (.a1.bb == set(.b1));
            assert (.a2.bb == set(.b1));
            assert (.a3.bb == set());
            assert (.a4.bb == set());

            .a1.aa |= set(.a1, .a2);
            assert (.a1.a == .a1);
            assert (.a2.a == .a1);
            assert (.a1.aa == set(.a1, .a2));
            assert (.a2.aa == set());

            .a2.aa |= set(.a1, .a2);
            assert (.a1.a == .a2);
            assert (.a2.a == .a2);
            assert (.a1.aa == set());
            assert (.a2.aa == set(.a1, .a2));

            .a2.aa.remove(||true);
            assert (.a1.a == nil);
            assert (.a2.a == nil);
            assert (.a1.aa == set());
            assert (.a2.aa == set());

            .a1.aaa = set(.a1, .a2);
            assert (.a1.aaa == set(.a1, .a2));
            assert (.a2.aaa == set(.a1));

            .a1.aaa.remove(||true);
            assert (.a1.aaa == set());
            assert (.a2.aaa == set());

            .a2.aa |= set(.a1, .a2);
            assert (.a1.a == .a2);
            assert (.a2.a == .a2);
            assert (.a1.aa == set());
            assert (.a2.aa == set(.a1, .a2));

            .a2.aa.clear();
            assert (.a1.a == nil);
            assert (.a2.a == nil);
            assert (.a1.aa == set());
            assert (.a2.aa == set());

            .a1.aaa = set(.a1, .a2);
            assert (.a1.aaa == set(.a1, .a2));
            assert (.a2.aaa == set(.a1));

            .a1.aaa.clear();
            assert (.a1.aaa == set());
            assert (.a2.aaa == set());

            .a2.aa |= set(.a1, .a2);
            assert (.a1.a == .a2);
            assert (.a2.a == .a2);
            assert (.a1.aa == set());
            assert (.a2.aa == set(.a1, .a2));

            'OK';
        '''), 'OK')

        await asyncio.sleep(0.2)

        for client in (client0, client1):
            self.assertEqual(await client.query(r'''
                assert (.a1.bb == set(.b1));
                assert (.a2.bb == set(.b1));
                assert (.a3.bb == set());
                assert (.a4.bb == set());

                assert (.a1.a == .a2);
                assert (.a2.a == .a2);
                assert (.a1.aa == set());
                assert (.a2.aa == set(.a1, .a2));
                assert (.a1.aaa == set());
                assert (.a2.aaa == set());

                'OK';
            '''), 'OK')

        self.assertEqual(await client0.query(r'''
            .a2.aa ^= set(.a1, .a3);
            assert (.a1.a == nil);
            assert (.a2.a == .a2);
            assert (.a3.a == .a2);
            assert (.a2.aa == set(.a2, .a3));

            'OK';
        '''), 'OK')

        await asyncio.sleep(0.2)

        for client in (client0, client1):
            self.assertEqual(await client.query(r'''
                assert (.a1.a == nil);
                assert (.a2.a == .a2);
                assert (.a3.a == .a2);
                assert (.a2.aa == set(.a2, .a3));

                'OK';
            '''), 'OK')

    async def test_full_store(self, client0):
        await client0.query(r'''
            new_type('A');
            new_type('B');

            set_type('A', {
                a: 'A?',
                aa: '{A}',
                bb: '{B}',
            });

            set_type('B', {
                aa: '{A}'
            });

            mod_type('A', 'rel', 'a', 'aa');
            mod_type('A', 'rel', 'bb', 'aa');
        ''')

        self.assertEqual(await client0.query(r'''
            .a1 = A{};
            .a2 = A{};
            .a3 = A{};
            .a4 = A{};

            .b1 = B{};
            .b2 = B{};

            .b1.aa.add(.a1, .a2, .a3);
            .b1.aa = set(.a2, .a3);
            .b1.aa |= set(.a3, .a4);
            .b1.aa ^= set(.a1, .a3, .a4);
            .a1.aa |= set(.a1, .a2);
            .a2.aa |= set(.a1, .a2);
            'OK';
        '''), 'OK')

        await self.node0.shutdown()
        await self.node0.run()
        await self.wait_nodes_ready()

        res = await client0.query(r'''
            [type_info('A').load().relations, type_info('B').load().relations];
        ''')

        self.assertEqual(res[0], {'a': {}, 'aa': {}, 'bb': {}})
        self.assertEqual(res[1], {'aa': {}})
        self.assertEqual(await client0.query(r'''
            assert (.a1.bb == set(.b1));
            assert (.a2.bb == set(.b1));
            assert (.a3.bb == set());
            assert (.a4.bb == set());

            assert (.a1.a == .a2);
            assert (.a2.a == .a2);
            assert (.a1.aa == set());
            assert (.a2.aa == set(.a1, .a2));

            'OK';
        '''), 'OK')

    async def test_relation_init_non_id(self, client):
        await client.query(r'''
            new_type('P');
            new_type('W');
            new_type('S');
            set_type('P', {
                w: 'W?'
            });
            set_type('W', {
                p: '{P}'
            });
            set_type('S', {
                s: '{S}'
            });
            .w = W{};
            .p = P{};
            .s = S{};
        ''')

        err_msg = (
            r'failed to create relation; '
            r'relations must be created using a property on a stored '
            r'thing \(a thing with an Id\)')

        with self.assertRaisesRegex(TypeError, err_msg):
            await client.query(r'''
                w = W{
                    p: set(.p)
                };
                mod_type('W', 'rel', 'p', 'w');
            ''')

        with self.assertRaisesRegex(TypeError, err_msg):
            await client.query(r'''
                p = P{
                    w: .w
                };
                mod_type('W', 'rel', 'p', 'w');
            ''')

        with self.assertRaisesRegex(TypeError, err_msg):
            await client.query(r'''
                s = S{
                    s: set(.s)
                };
                mod_type('S', 'rel', 's', 's');
            ''')

    async def test_iteration_id(self, client):
        await client.query(r'''
            new_type('P');
            new_type('W');
            set_type('P', {
                w: '{W}'
            });
            set_type('W', {
                p: '{P}'
            });
            mod_type('W', 'rel', 'p', 'w');
        ''')
        await client.query(r'''
            .w = W{};
            .w.p.add(P{}, P{}, P{});
        ''')
        self.assertEqual(await client.query(r'''
            w = .w;
            .w.p.map(|p| p.w.remove(w));
             'OK';
        '''), 'OK')
        await client.query(r'''
            .w = W{};
            .w.p.add(P{}, P{}, P{});
        ''')
        self.assertEqual(await client.query(r'''
            w = .w;
            .w.p.each(|p| p.w.remove(w));
             'OK';
        '''), 'OK')
        await client.query(r'''
            .w = W{};
            .w.p.add(P{}, P{}, P{});
        ''')
        self.assertEqual(await client.query(r'''
            w = .w;
            .w.p.filter(|p| p.w.add(W{}));
             'OK';
        '''), 'OK')

        self.assertEqual(await client.query(r'''
            w = .w;
            .w.p.filter(|p| p.w.remove(w));
             'OK';
        '''), 'OK')

        self.assertEqual(await client.query(r'''
            w = W{};
            .p = P{};
            .p.w.add(w);

            return [.p.w.len(), w.p.len()];
        '''), [1, 1])

    async def test_iteration_tset(self, client):
        await client.query(r'''
            new_type('P');
            new_type('W');
            set_type('P', {
                w: 'W?'
            });
            set_type('W', {
                p: '{P}'
            });
            mod_type('W', 'rel', 'p', 'w');
        ''')

        self.assertEqual(await client.query(r'''
            w = W{};
            .p = P{};
            .p.w = w;
            return w.p.len()
        '''), 1)

        self.assertEqual(await client.query(r'''
            p = P{};
            .w = W{};
            .w.p.add(p);
            return p.w.len()
        '''), 1)

    async def test_wse_on_closure(self, client):
        await client.query("""//ti
            new_type('A');
            new_type('B');

            set_type('A', {
                b: '{B}'
            });

            set_type('B', {
                a: '{A}'
            });

            mod_type('A', 'rel', 'b', 'a');

            .a = A{};
            .b = B{};
            .b.a.add(.a);
            .clr = || (||.a.b.clear()).call();
            .check = || {
                .b.a.each(|| .clr());
            };
        """)

        await self.node0.shutdown()
        await self.node0.run()
        await self.wait_nodes_ready()

        with self.assertRaises(OperationError):
            # see pr #303
            _res = await client.query("""//ti
                wse();
                .check();
            """)

    async def test_set_operation_complex(self, client0):
        if not self.with_node1():
            return
        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')

        await client0.query("""//ti
            set_type('P', {name: 'str'});
            set_type('S', {p: '{P}'});
            set_type('R', {name: 'str', f:'{R}'});
            mod_type('R', 'rel', 'f', 'f');

            .iris = {name: 'iris'};
            .cato = {name: 'cato'};
            .tess = {name: 'tess'};

            .Iris = P{name: 'Iris'};
            .Cato = P{name: 'Cato'};
            .Tess = P{name: 'Tess'};

            .s1 = set(.iris, .tess);
            .s2 = set(.iris, .cato);
            .s3 = set(.cato, .tess);

            .S1 = S{p: set(.Iris, .Tess)};
            .S2 = S{p: set(.Iris, .Cato)};
            .S3 = S{p: set(.Cato, .Tess)};

            .RIris = R{name: 'Iris', f: set()};
            .RCato = R{name: 'Cato', f: set()};
            .RTess = R{name: 'Tess', f: set()};
        """)

        with self.assertRaisesRegex(TypeError, r'another type'):
            await client0.query("""//ti
                .S1.p ^= .s2;
            """)

        with self.assertRaisesRegex(TypeError, r'a thing with an Id'):
            await client0.query("""//ti
                r = R{}; r.f |= set(.RIris);  // thing without an Id
            """)

        await client0.query("""//ti
            .sp = S{};
            .so = S{};
            .so.p |= .S2.p;
            assert (.so.p == .S2.p);
        """)

        await client0.query("""//ti
            .ro = R{name: 'other'};
            .ro.f |= set(.RIris, .RCato);
            assert (.RIris.f.has(.ro));
            assert (.RCato.f.has(.ro));
        """)

        with self.assertRaisesRegex(OperationError, r'enforce a change'):
            await client0.query("""//ti
                s = .sp.p;
                s |= (.S2.p - .S3.p);
                assert (.sp.p == set(.Iris));
            """)

        await client0.query("""//ti
            wse();
            s = .sp.p;
            s |= (.S2.p - .S3.p);
            assert (.sp.p == set(.Iris));
        """)

        await client0.query("""//ti
            s1 = .ro.f & set(.RIris, .RCato, .RTess);
            s2 = set(.RIris, .RCato, .RTess) | .ro.f;
            s3 = set(.RIris, .RCato, .RTess) - .ro.f;
            assert (s1 == set(.RIris, .RCato));
            assert (s2 == set(.RIris, .RCato, .RTess));
            assert (s3 == set(.RTess));
        """)

        await asyncio.sleep(0.2)
        await self.wait_nodes_ready()

        for client in (client0, client1):
            await client.query(r"""//ti
                assert (.RIris.f.has(.ro));
                assert (.RCato.f.has(.ro));
                assert (.ro.f = set(.RIris, .RCato));
                assert (.sp.p == set(.Iris));
            """)

    async def test_strict_rel(self, client):
        await client.query(r"""//ti
            new_type("A");
            new_type("B");
            new_type("C");
            set_type("A", {one: 'A?'});
            set_type("B", {one: 'B?', many: '{B}'});
            set_type("C", {many: '{C}'});
            mod_type("A", "rel", "one", "one");
            mod_type("B", "rel", "one", "many");
            mod_type("C", "rel", "many", "many");
        """)

        with self.assertRaisesRegex(
                TypeError,
                r'mismatch in type `A` on property `one`; '
                r'relations must be created using a property on '
                r'a stored thing \(a thing with an Id\)'):
            await client.query(r"""//ti
                a = A{};
                a.one = a;
            """)

        with self.assertRaisesRegex(
                TypeError,
                r'mismatch in type `A` on property `one`; '
                r'relations must be created using a property on '
                r'a stored thing \(a thing with an Id\)'):
            await client.query(r"""//ti
                a = A{};
                a.set("one", a);
            """)

        with self.assertRaisesRegex(
                TypeError,
                r'mismatch in type `B` on property `one`; '
                r'relations must be created using a property on '
                r'a stored thing \(a thing with an Id\)'):
            await client.query(r"""//ti
                b = B{};
                b.one = b;
            """)

        with self.assertRaisesRegex(
                TypeError,
                r'mismatch in type `B` on property `many`; '
                r'relations must be created using a property on '
                r'a stored thing \(a thing with an Id\)'):
            await client.query(r"""//ti
                b = B{};
                b.many |= set(b);
            """)

        with self.assertRaisesRegex(
                TypeError,
                r'relations must be created using a property on '
                r'a stored thing \(a thing with an Id\)'):
            await client.query(r"""//ti
                c = C{};
                c.many.add(c);
            """)

        await client.query(r"""//ti
            a = .a = A{};
            a.set("one", a);
            assert(a.one == a);
        """)

        await client.query(r"""//ti
            b = .b = B{};
            b.set("one", b);
            assert(b.many.has(b));
        """)

        await client.query(r"""//ti
            b = .b = B{};
            b.many |= set(b);
            assert(b.one == b);
        """)

        await client.query(r"""//ti
            c = .c = C{};
            c.many.add(c);
            assert(c.many.has(c));
        """)

        # Test rename type with relation

        await client.query("""//ti
            rename_type('A', 'AA');
            rename_type('B', 'BB');
            rename_type('C', 'CC');
        """)

        await self.node0.shutdown()
        await self.node0.run()

        res = await client.query("""//ti
            type_info("AA").load().fields;
        """)
        self.assertEqual(res, [['one', 'AA?']])
        res = await client.query("""//ti
            type_info("BB").load().fields;
        """)
        self.assertEqual(res, [['one', 'BB?'], ['many', '{BB}']])
        res = await client.query("""//ti
            type_info("CC").load().fields;
        """)
        self.assertEqual(res, [['many', '{CC}']])


if __name__ == '__main__':
    run_test(TestRelations())

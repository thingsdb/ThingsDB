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
from thingsdb.exceptions import SyntaxError
from thingsdb.exceptions import TypeError
from thingsdb.exceptions import NumArgumentsError
from thingsdb.exceptions import BadDataError
from thingsdb.exceptions import LookupError
from thingsdb.exceptions import OverflowError
from thingsdb.exceptions import ZeroDivisionError
from thingsdb.exceptions import OperationError


class TestRestriction(TestBase):

    title = 'Test value restriction'

    @default_test_setup(num_nodes=1, seed=1, threshold_full_storage=10)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_array_restriction(self, client):
        # bug #239
        with self.assertRaisesRegex(
                TypeError,
                "type `str` is not allowed in restricted array"):
            await client.query(r"""//ti
                set_type('T', {arr: '[int]'});
                T{}.arr.push('test');
            """)

    async def test_restriction_type(self, client):
        await client.query(r"""//ti
            set_type('X', {lookup: 'thing<float>'});
            .x = X{};
            .x.lookup.pi = 3.14;
        """)

        with self.assertRaisesRegex(TypeError, "restriction mismatch"):
            await client.query('.x.lookup.name = "test";')

        with self.assertRaisesRegex(TypeError, "restriction mismatch"):
            await client.query('X{}.lookup.name = "test";')

        with self.assertRaisesRegex(TypeError, "restriction mismatch"):
            await client.query(r"""//ti
                t = X{lookup: thing()};
                t.lookup.name = "test";
            """)

        res = await client.query(r"""//ti
            .t = X{};
            .t.lookup = thing();
            'OK';
        """)
        self.assertEqual(res, 'OK')

        with self.assertRaisesRegex(TypeError, "restriction mismatch"):
            await client.query(r"""//ti
                .t.lookup.name = "test";
            """)

        with self.assertRaisesRegex(
                ValueError,
                "mismatch in type `X` on property `lookup`; "
                "restriction mismatch"):
            await client.query(r"""//ti
                .t.lookup = thing().restrict('room')
            """)

        res = await client.query(r"""//ti
            .t.lookup = thing().restrict('float');
            'OK';
        """)
        self.assertEqual(res, 'OK')

        res = await client.query(r"""//ti
            .del('t');
            assert (.x.lookup.restriction() == 'float');

            mod_type(
                'X',
                'mod',
                'lookup',
                'thing<int>',
                |x| {pi: int(x.lookup.pi)});

            assert (.x.lookup.len() == 1);
            assert (.x.lookup.pi == 3);
            assert (.x.lookup.restriction() == 'int');

            mod_type(
                'X',
                'mod',
                'lookup',
                'thing<X>',
                || thing());

            assert (.x.lookup.len() == 0);
            assert (.x.lookup.restriction() == 'X');

            'OK';
        """)
        self.assertEqual(res, 'OK')

    async def test_restriction_migration(self, client):
        res = await client.query(r"""//ti
            set_type('N', {name: 'str'});
            set_type('L', {lookup: 'thing'});
            .to_type('L');
            .lookup.iris = N{name: 'Iris'};
            .lookup.cato = N{name: 'Cato'};
            .lookup.tess = N{name: 'Tess'};
            'OK';
        """)
        self.assertEqual(res, 'OK')

        res = await client.query(r"""//ti
            mod_type('L', 'mod', 'lookup', 'thing<N>', ||nil);
            assert (.lookup.iris.name == 'Iris');
            assert (.lookup.cato.name == 'Cato');
            assert (.lookup.tess.name == 'Tess');

            err = try(.lookup.number = 123);
            assert (is_err(err));

            'OK';
        """)
        self.assertEqual(res, 'OK')

    async def test_restrict_function(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `list` has no function `restrict`'):
            await client.query('[].restrict();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `restrict` takes 1 argument but 0 were given'):
            await client.query('{}.restrict();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `restrict` takes 1 argument but 2 were given'):
            await client.query('{}.restrict(nil, nil);')

        with self.assertRaisesRegex(
                TypeError,
                'function `restrict` expects argument 1 to be of '
                'type `str` or `nil` but got type `int` instead'):
            await client.query('{}.restrict(123);')

        with self.assertRaisesRegex(
                ValueError,
                "current restriction is enforced by at least one type"):
            await client.query(r"""//ti
                a = {};
                set_type('X', {strict: 'thing<int>'});
                b = X{strict: a};
                a.restrict(nil);
            """)

        res = await client.query(r"""//ti
            a = {};
            set_type('Y', {strict: 'thing<int>'});
            b = Y{strict: a};
            del_type('Y');
            a.restrict(nil);
            'OK';
        """)

        self.assertEqual(res, 'OK')


if __name__ == '__main__':
    run_test(TestRestriction())

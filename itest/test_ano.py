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


class TestAno(TestBase):

    title = 'Test anonymous type'

    @default_test_setup(num_nodes=1, seed=1, threshold_full_storage=10)
    async def async_run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        await self.run_tests(client.query)

        client.close()
        await client.wait_closed()

    async def test_ano_err(self, q):
        await q(r"""//ti
            set_type('A', {x: 'any'});
        """)

        res = await q("""//ti
                .x = {}.wrap(&{});
                .x;
        """)
        self.assertIs(res, None)

        with self.assertRaisesRegex(
                LookupError,
                r'function `ano` is undefined in the `@thingsdb` scope; '
                r'you might want to query a `@collection` scope\?;'):
            await q("""//ti
                ano({});
            """, scope='/t')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `ano` takes 1 argument but 0 were given'):
            await q("""//ti
                ano();
            """)

        with self.assertRaisesRegex(
                TypeError,
                r'function `ano` expects argument 1 to be of '
                r'type `thing` but got type `nil` instead;'):
            await q("""//ti
                ano(nil);
            """)

        with self.assertRaisesRegex(
                TypeError,
                r'expecting a nested structure \(`list` or `thing`\), ' \
                r'a method of type `closure` or a definition '
                r'of type `str` but got type `int` instead'):
            await q(
                """//ti
                ano({
                    x: 0
                });
            """)

        with self.assertRaisesRegex(
                LookupError,
                r'anonymous types are not supported in the the `@thingsdb` '
                r'scope'):
            await q("""//ti
                &{};
            """, scope='/t')

    async def test_more_ano_props(self, q):
        r = await q("""//ti
                    .a = &{
                        id: '#',
                        a: 'int',
                        b: 'str',
                        f: |a, b| a + b,
                    };
                    .a.f(20, 22);
                    """)
        self.assertEqual(r, 42)
        r1, r2 = await q("""//ti
                o = {
                    name: 'Iris',
                    numbers: [{
                        x: 1,
                        y: 2,
                    }, {
                        x: 4,
                    }, {
                        y: 5,
                    }]
                };
                [
                    o.wrap(&{
                        name: |this| this.name.upper(),
                        numbers: [{
                            x: 'int'
                        }]
                    }),
                    o.wrap(ano({
                        name: |this| this.name.upper(),
                        numbers: [{
                            x: 'int'
                        }]
                    })),
                ];
                """
            )
        self.assertEqual(r1, {
            "name": "IRIS",
            "numbers": [{"x": 1,}, {"x": 4}, {}]
        })
        self.assertEqual(r1, r2)
        await q("""//ti
                o = {
                    name: 'Iris',
                    numbers: [{
                        x: 1,
                        y: 2,
                    }, {
                        x: 4,
                    }, {
                        y: 5,
                    }]
                };
                .wano = [
                    o.wrap(&{
                        name: |this| this.name.upper(),
                        numbers: [{
                            x: 'int'
                        }]
                    }),
                    o.wrap(ano({
                        name: |this| this.name.upper(),
                        numbers: [{
                            x: 'int'
                        }]
                    })),
                ];
                """
            )
        r1, r2 = await q("""//ti
                         .wano;
                         """)
        self.assertEqual(r1, {
            "name": "IRIS",
            "numbers": [{"x": 1,}, {"x": 4}, {}]
        })
        self.assertEqual(r1, r2)
        r = await q(
            """//ti
            .wano[0] == .wano[1]
            """)
        self.assertTrue(r)




if __name__ == '__main__':
    run_test(TestAno())

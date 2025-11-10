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

        await self.run_tests(client.query, client.run)

        client.close()
        await client.wait_closed()

    async def test_ano_err(self, q, r):
        await q(r"""//ti
            set_type('A', {x: 'any'});
        """)

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

        with self.assertRaisesRegex(
                TypeError,
                r'the <anonymous> type definition contains dependencies '
                r'on other types or enumerators, which is not permitted; '
                r'to resolve this, create a fully named wrap-only type'):
            await q("""//ti
                new_type('T');
                &{nested: {t: 'T'}};
            """)

    async def test_more_ano_props(self, q, r):
        res = await q("""//ti
                    .a = &{
                        id: '#',
                        a: 'int',
                        b: 'str',
                        f: |a, b| a + b,
                    };
                    .a.f(20, 22);
                    """)
        self.assertEqual(res, 42)
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
                .a1 = &{
                    name: |this| this.name.upper(),
                    numbers: [{
                        x: 'int'
                    }]
                };
                .a2 = ano({
                    name: |this| this.name.upper(),
                    numbers: [{
                        x: 'int'
                    }]
                });
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
                    o.wrap(.a1),
                    o.wrap(.a2),
                ];
                """
            )
        r1, r2, r3, r4 = await q("""//ti
                         .wano;
                         """)
        self.assertEqual(r1, {
            "name": "IRIS",
            "numbers": [{"x": 1,}, {"x": 4}, {}]
        })
        self.assertEqual(r1, r2)
        self.assertEqual(r2, r3)
        self.assertEqual(r3, r4)
        res = await q(
            """//ti
            .wano[0] == .wano[1] &&
            .wano[1] == .wano[2] &&
            .wano[2] == .wano[3];

            """)
        self.assertTrue(res)

        res = await q("""//ti
                type(&{});
                """)
        self.assertEqual(res, "<anonymous>")

        res = await q("""//ti
                type({}.wrap(&{}));
                """)
        self.assertEqual(res, "<<anonymous>>")

        res = await q("""//ti
                .a1;
                """)
        self.assertEqual(res, {
            'name': '|this|this.name.upper()',
            'numbers': [
                {'x': 'int'}
            ]
        })

        res = await q("""//ti
                str(.a1);
                """)
        self.assertEqual(res, "<anonymous>")

        res = await q("""//ti
                str({}.wrap(.a1));
                """)
        self.assertEqual(res, "<<anonymous>>:nil")

    async def test_after_mod(self, q, r):
        await q("""//ti
                set_type('P', {name: 'str'});
                .p = P{name: 'foo'};
                .a = &{name: 'str', age: 'int', other: 'any'};
                new_procedure('get', || {
                    [.p.wrap(&{name: 'any', age: 'any'}), .p.wrap(.a)];
                });
                """)
        script = """//ti
                '
This is a query with will be cached as it is long enough to be stored,
it should have at least 160 characters, so we need a longs string.
I guess we are good now...
';
[.p.wrap(&{name: 'str', age: 'int'}), .p.wrap(.a)];
                """
        r0, r1 = await q(script)
        self.assertEqual(r0, {"name": "foo"})
        self.assertEqual(r0, r1)
        await q("""//ti
                mod_type('P', 'add', 'age', 'int', 12);
                """)
        r0, r1 = await q(script)
        self.assertEqual(r0, {"name": "foo", "age": 12})
        self.assertEqual(r0, r1)
        r0, r1 = await r('get')
        self.assertEqual(r0, {"name": "foo", "age": 12})
        self.assertEqual(r0, r1)

        res = await q(
            """//ti
            range(3).map(|i| P{name: `person{i}`, age: i}).map_wrap(&{
                upper: |this| this.name.upper(),
                age: 'int'
            });
            """)
        self.assertEqual(res, [
            {'age': 0, 'upper': 'PERSON0'},
            {'age': 1, 'upper': 'PERSON1'},
            {'age': 2, 'upper': 'PERSON2'}])

        await q("""//ti
            t = &{name: 'str'};
            .w = {nested: {t: {name: 'foo'}}}.wrap(&{nested: {t:,}});
        """)
        res = await q('.w;')
        self.assertEqual(res, {"nested": {'t': {'name': 'foo'}}})



if __name__ == '__main__':
    run_test(TestAno())

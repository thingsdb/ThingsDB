#!/usr/bin/env python
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client
from thingsdb.client import Client
from thingsdb.exceptions import AssertionError
from thingsdb.exceptions import LookupError
from thingsdb.exceptions import NumArgumentsError
from thingsdb.exceptions import OperationError
from thingsdb.exceptions import SyntaxError
from thingsdb.exceptions import TypeError
from thingsdb.exceptions import ValueError


class TestTemplate(TestBase):

    title = 'Test template'

    @default_test_setup(num_nodes=1, seed=1, threshold_full_storage=10)
    async def async_run(self):
        self.maxDiff = 1000
        self.node0.version()

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_template(self, client: Client):
        q = client.query

        await q("""//ti
            set_type('Person', {
                name: 'str',
                age: 'int',
            });
            set_type('TplHolder', {
                tpl: 'template',
                opt_tpl: 'template?',
            });
        """)

        # --- Type name, default value, truthiness (bool), str(), ... ---

        res = await q("""//ti
            t_empty = template('');
            assert(type(t_empty) == 'template');
            assert(is_template(t_empty));
            assert(t_empty == template(''));
            assert(t_empty.len() == 0);
            assert(bool(t_empty) == false);
            assert(!t_empty);
            assert(type(str(t_empty)) == 'str');
            assert(str(t_empty) == '');

            t = template('Hello {name}!');
            assert(type(t) == 'template');
            assert(t.len() == 13);
            assert(bool(t) == true);
            assert(!!t);
            assert(type(str(t)) == 'str');
            assert(str(t) == 'Hello {name}!');
            t;
        """)
        self.assertEqual(res, 'Hello {name}!')

        # --- Equality & unsupported comparisons (<, <=, >, >=) ---

        await q("""//ti
            t1 = template('a={a}');
            t2 = template('a={a}');
            t3 = template('b={b}');

            assert(t1 == t2);
            assert(t1 != t3);
            assert(!(t1 == t3));
            assert(t1 != 'a={a}');
        """)

        for op in ('<', '<=', '>', '>='):
            with self.assertRaisesRegex(
                        TypeError,
                        r"`..?` not supported "
                        r"between `template` and `template`"):
                await q(f"""//ti
                    template('a') {op} template('b');
                """)

        # --- Slicing should NOT work ---

        with self.assertRaisesRegex(
                    TypeError,
                    r"type `template` has no slice support"):
            await q("""//ti
                t = template('0123456789');
                t[4:9];
            """)

        # --- Invalid templates & argument validation ---

        with self.assertRaisesRegex(
                    SyntaxError,
                    r"error at line 1, position 16, "
                    r"unexpeced end of template"):
            await q("""//ti
                template('Hello {unclosed');
            """)

        with self.assertRaisesRegex(
                    SyntaxError,
                    r"error at line 1, position 12, "
                    r"unexpected character `}`"):
            await q("""//ti
                template('Hello {1 + }');
            """)

        with self.assertRaisesRegex(
                    TypeError,
                    r"function `template` expects argument 1 to be of "
                    r"type `str` but got type `int` instead;"):
            await q("""//ti
                template(123);
            """)

        with self.assertRaisesRegex(
                    NumArgumentsError,
                    r"function `template` takes 1 argument but 2 were given;"):
            await q("""//ti
                template('a', 'b');
            """)

        # --- Render without scope argument (uses current scope) ---

        res = await q("""//ti
            name = 'Alice';
            count = 3;
            t = template('{name} has {count * 2} apples and {{braces}}.');
            t.render();
        """)
        self.assertEqual(res, 'Alice has 6 apples and {braces}.')

        # --- Render with plain thing & typed instance scope ---

        res = await q("""//ti
            t = template('{name} is {age} years old');
            p = Person{name: 'Bob', age: 42};
            [
                t.render({name: 'Charlie', age: 30}),
                t.render(p),
            ];
        """)
        self.assertEqual(res, [
            'Charlie is 30 years old',
            'Bob is 42 years old',
        ])

        # --- Render with illegal scope ---

        with self.assertRaisesRegex(
                    ValueError,
                    r"cannot set property `` as scope variable: "
                    r"invalid variable name"):
            await q("""//ti
                t = template('hello');
                d = {};
                d[''] = 5;
                t.render(d);
            """)

        with self.assertRaisesRegex(
                    ValueError,
                    r"cannot set property `invalid-name` as scope variable: "
                    r"invalid variable name"):
            await q("""//ti
                t = template('hello');
                d = {};
                d['invalid-name'] = 5;
                t.render(d);
            """)

        # --- Render with dict-backed thing after deleting incompatible key ---

        res = await q("""//ti
            t = template('x={x}');
            d = {};
            d[''] = 5;
            d['x'] = 99;
            d.del('');
            t.render(d);
        """)
        self.assertEqual(res, 'x=99')

        # --- Recursive render protection ---

        with self.assertRaisesRegex(
                    OperationError,
                    r"cannot render a template recursively"):
            await q("""//ti
                .t = template('{.t.render()}');
                .t.render();
            """)

        # --- Changes (with change vs without change) ---

        # Rendering without mutating stored data requires no change
        res = await q("""//ti
            .stored_tpl = template('Value: {x}');
            .stored_tpl.render({x: 10});
        """)
        self.assertEqual(res, 'Value: 10')

        # Rendering an expression that mutates a stored thing creates a change
        await q("""//ti
            .counter = 0;
            t = template('{.counter += 1}');
            assert(t.render() == '1');
            assert(t.render() == '2');
        """)
        res = await q("""//ti
            .counter;
        """)
        self.assertEqual(res, 2)

        with self.assertRaisesRegex(
                    SyntaxError,
                    r"xxx"):
            await q("""//ti
                template('{.counter += 1}').render();
            """)


        await q("""//ti
            h_default = TplHolder{};
            assert(type(h_default.tpl) == 'template');
            assert(h_default.tpl == template(''));
            assert(h_default.tpl.len() == 0);
            assert(h_default.opt_tpl == nil);

            h_direct = TplHolder{
                tpl: template('Hi {who}'),
                opt_tpl: template('Bye {who}'),
            };
            assert(h_direct.tpl.render({who: 'Ada'}) == 'Hi Ada');
            assert(h_direct.opt_tpl.render({who: 'Ada'}) == 'Bye Ada');

            h_conv = TplHolder({
                tpl: 'Converted {x}',
                opt_tpl: 'Optional {x}',
            });
            assert(type(h_conv.tpl) == 'template');
            assert(h_conv.tpl.render({x: 7}) == 'Converted 7');
            assert(type(h_conv.opt_tpl) == 'template');
            assert(h_conv.opt_tpl.render({x: 7}) == 'Optional 7');

            h_conv.assign({tpl: 'Assigned {y}'});
            assert(type(h_conv.tpl) == 'template');
            assert(h_conv.tpl.render({y: 8}) == 'Assigned 8');
        """)

        # Strict literal initialization must reject raw str
        with self.assertRaisesRegex(
                TypeError,
                r"mismatch in type `TplHolder`; type `str` is invalid for "
                r"property `tpl` with definition `template`"):
            await q("""//ti
                TplHolder{tpl: 'raw string not allowed without conversion'};
            """)

        # Invalid type or invalid template syntax during auto-conversion
        with self.assertRaisesRegex(
                    TypeError,
                    r"mismatch in type `TplHolder`; type `int` is invalid "
                    r"for property `tpl` with definition `template`"):
            await q("""//ti
                TplHolder({tpl: 123});
            """)

        with self.assertRaisesRegex(
                    SyntaxError,
                    r"error at line 1, position 12, "
                    r"unexpeced end of template"):
            await q("""//ti
                TplHolder({tpl: 'bad {syntax'});
            """)


if __name__ == '__main__':
    run_test(TestTemplate())
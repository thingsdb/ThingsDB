#!/usr/bin/env python
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client
from thingsdb.exceptions import ValueError
from thingsdb.exceptions import LookupError
from thingsdb.exceptions import NumArgumentsError
from thingsdb.exceptions import OperationError
from thingsdb.exceptions import TypeError


dump = None


class TestImport(TestBase):

    title = 'Test export and import'

    @default_test_setup(num_nodes=2, seed=1, threshold_full_storage=500)
    async def run(self):

        await self.node0.init_and_run()

        client0 = await get_client(self.node0)
        client0.set_default_scope('//stuff')

        # add another node for query validation
        if hasattr(self, 'node1'):
            await self.node1.join_until_ready(client0)
            client1 = await get_client(self.node1)
            client1.set_default_scope('//stuff')
        else:
            client1 = client0

        await self.run_tests(client0, client1)

    async def test_export_err(self, client0, client1):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `export`'):
            await client0.query('nil.export();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `export` takes at most 1 argument '
                'but 2 were given'):
            await client0.query('export(nil, nil);')

        with self.assertRaisesRegex(
                TypeError,
                'function `export` expects argument 1 to be of '
                'type `thing` but got type `int` instead;'):
            await client0.query('export(123);')

        with self.assertRaisesRegex(
                ValueError,
                'invalid export option `unknown`'):
            await client0.query('export({unknown: true});')

        with self.assertRaisesRegex(
                TypeError,
                'dump must be of type `bool` but got type `int` instead;'):
            await client0.query('export({dump: 1});')

        with self.assertRaisesRegex(
                OperationError,
                'function `export` must not be used with a change or future'):
            await client0.query('wse(); export({dump: true});')

    async def test_import_err(self, client0, client1):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `import`'):
            await client0.query('nil.import();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `import` takes at most 2 arguments '
                'but 3 were given'):
            await client0.query('import(nil, nil, nil);')

        with self.assertRaisesRegex(
                TypeError,
                'function `import` expects argument 1 to be of '
                'type `bytes` but got type `int` instead;'):
            await client0.query('import(123);')

        with self.assertRaisesRegex(
                TypeError,
                'function `import` expects argument 2 to be of '
                'type `thing` but got type `int` instead;'):
            await client0.query('import(bytes(), 123);')

        with self.assertRaisesRegex(
                ValueError,
                'invalid import option `unknown`'):
            await client0.query('import(bytes(), {unknown: true});')

        with self.assertRaisesRegex(
                TypeError,
                'import_tasks must be of type `bool` but got '
                'type `int` instead;'):
            await client0.query('import(bytes(), {import_tasks: 1});')

        with self.assertRaisesRegex(
                ValueError,
                'no valid import data; make sure the bytes are created '
                'using the `export` function with the `dump` option enabled'):
            await client0.query('import(bytes());')

        with self.assertRaisesRegex(
                OperationError,
                'collection not empty; '
                'collection `stuff` contains properties'):
            await client0.query('.x = 1; import(bytes());')

    async def test_export(self, client0, client1):
        global dump
        await client0.query(r"""//ti
            .x = 42;
            .t = task(datetime(), |t, r| {
                log(`running task for root Id {r.id()}...`);
                t.again_in('seconds', 1);
            }, [root()]);
            .me = root();
            new_procedure('test', ||nil);
            set_enum('E', {
                A: 1,
                B: 2,
                repr: |this| `{this.name()}:{this.value()}`,
            });
            set_enum('EE', {
                TA: {a: E{A}},
                TB: {b: E{B}},
            });
            new_type('T');
            new_type('R');
            set_type('R', {
                name: 'str',
                parent: '{R}',
                r: 'R?'
            });
            set_type('T', {
                x: 'int',
                t: 'task',
                me: 'thing',
                e: 'E',
                tlist: '[EE]',
                r: 'R',
            });
            mod_type('R', 'rel', 'parent', 'r');
            .to_type('T');
            mod_type('T', 'mod', 'me', 'T?', |t| t);
            .tlist.push(EE{TA}, EE{TB});  // bug found ans solved in #357
            .r.name = 'master';
            .r.r = R{name: 'slave'};
        """)

        dump = await client0.query(r"""//ti
            export({
                dump: true
            });
        """)

    async def test_import(self, client0, client1):
        await client0.query(r"""//ti
            import(dump);
        """, dump=dump)

        for client in (client0, client1):
            await client.query(r"""//ti
                wse();
                assert(.x == 42);
                assert(.r.name == 'master');
                assert(.r.r.name == 'slave');
                assert(.r.r.parent.one().name == 'master');
                assert(.me.id() == .id());
                assert(.e.repr() == 'A:1');
                assert(.tlist[0].value().a.repr() == 'A:1');
                assert(.tlist[1].value().b.repr() == 'B:2');
                assert(is_nil(.t.id()));
                assert(procedures_info().len() == 1);
            """)

    async def test_import_tasks(self, client0, client1):
        await client0.query(r"""//ti
            import(dump, {import_tasks: true});
        """, dump=dump)

        for client in (client0, client1):
            await client.query(r"""//ti
                wse();
                assert(.x == 42);
                assert(.r.name == 'master');
                assert(.r.r.name == 'slave');
                assert(.r.r.parent.one().name == 'master');
                assert(.me.id() == .id());
                assert(.e.repr() == 'A:1');
                assert(.tlist[0].value().a.repr() == 'A:1');
                assert(.tlist[1].value().b.repr() == 'B:2');
                assert(is_int(.t.id()));
                assert(procedures_info().len() == 1);
            """)

    async def test_nested_import(self, client0, client1):
        await client0.query(r"""//ti
            .msg = 'Hello world!';
        """)
        data = await client0.query(r"""//ti
            export({
                dump: true
            });
        """)

        with self.assertRaisesRegex(
                OperationError,
                'collection `stuff` is being used'):
            await client0.query(r"""//ti
                .del('msg');
                .other = {
                    import(data);
                };
            """, data=data)

        await client0.query(r"""//ti
            .msg = 'Hi!';
            .del('msg');
            import(data);
            .other = 'Hello';
        """, data=data)

        for client in (client0, client1):
            await client.query(r"""//ti
                wse();
                assert(.other == 'Hello');
                assert(.msg == 'Hello world!');
            """)


if __name__ == '__main__':
    run_test(TestImport())

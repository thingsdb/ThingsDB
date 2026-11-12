#!/usr/bin/env python
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client
from thingsdb.client import Client
from thingsdb.room import Room
from thingsdb.exceptions import AssertionError
from thingsdb.exceptions import LookupError
from thingsdb.exceptions import MaxQuotaError
from thingsdb.exceptions import NumArgumentsError
from thingsdb.exceptions import OperationError
from thingsdb.exceptions import OverflowError
from thingsdb.exceptions import SyntaxError
from thingsdb.exceptions import TypeError
from thingsdb.exceptions import ValueError


class TestReadOnly(TestBase):

    title = 'Test read only properties'

    @default_test_setup(num_nodes=2, seed=1, threshold_full_storage=10)
    async def async_run(self):

        self.node0.version()

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        # add another node otherwise backups are not possible
        if hasattr(self, 'node1'):
            await self.node1.join_until_ready(client)

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_read_only(self, client: Client):
        await client.query("""//ti
            set_type('T', {
                name: 'str<::unknown>(ro)'
            });
            .t1 = T{};
            .t2 = T{name: 'iris'};
            .t3 = T({name: 'fleur'});
            .t4 = .t3.dup();
        """)
        names = await client.query("""//ti
            [.t1.name, .t2.name, .t3.name, .t4.name];
        """)
        self.assertEqual(names, ['unknown', 'iris', 'fleur', 'fleur'])
        with self.assertRaisesRegex(
                OperationError,
                r"mismatch in type `T`; property `name` is read-only"):
            await client.query("""//ti
                .t4.name = 'Nissa';
            """)
        with self.assertRaisesRegex(
                NumArgumentsError,
                r"function `mod_type` with task `mod` takes at most 4 "
                r"arguments when used on a read-only property; if a callback "
                r"is required, remove the read-only mark first and re-apply "
                r"read-only in a subsequent request; "):
            await client.query("""//ti
                mod_type('T', 'mod', 'name', 'str<::unknown>(ro)', |t| {
                    `{t.name[0].upper()}{t.name[1:]}`;
                });
            """)
        await client.query("""//ti
            mod_type('T', 'mod', 'name', 'str<::unknown>', |t| {
                `{t.name[0].upper()}{t.name[1:]}`;
            });
        """)

        with self.assertRaisesRegex(
                NumArgumentsError,
                r"function `mod_type` with task `mod` takes at most 4 "
                r"arguments when making a property read-only; if a callback "
                r"is required, make the changes first, then mark the "
                r"property read-only in a subsequent request;"):
            await client.query("""//ti
                mod_type('T', 'mod', 'name', 'str<::unknown>(ro)', |t| nil);
            """)

        await client.query("""//ti
            .t4.name = 'Nissa';
            mod_type('T', 'mod', 'name', 'str<::unknown>(ro)');
        """)
        names = await client.query("""//ti
            [.t1.name, .t2.name, .t3.name, .t4.name];
        """)
        self.assertEqual(names, ['Unknown', 'Iris', 'Fleur', 'Nissa'])
        with self.assertRaisesRegex(
                OperationError,
                r"mismatch in type `T`; property `name` is read-only"):
            await client.query("""//ti
                .t1.name = 'Unknown';
            """)
        res = await client.query("""//ti
            mod_type('T', 'add', 'key', 'str(ro)', 'secret123');
            .t1.key;
        """)
        self.assertEqual(res, "secret123")
        with self.assertRaisesRegex(
                OperationError,
                r"mismatch in type `T`; property `key` is read-only"):
            await client.query("""//ti
                .t1.key = 'xxx';
            """)
        await client.query("""//ti
            new_type('A');
            new_type('B');
            set_type('A', {
                a: 'A?(ro)',
                b: 'B?(ro)',
            });
            set_type('B', {
                a: 'A?',
            });
        """)
        with self.assertRaisesRegex(
                OperationError,
                r"failed to create relation; property `a` on "
                r"type `A` is read-only"):
            await client.query("""//ti
                mod_type('A', 'rel', 'a', 'a');
            """)
        with self.assertRaisesRegex(
                OperationError,
                r"failed to create relation; property `b` on "
                r"type `A` is read-only"):
            await client.query("""//ti
                mod_type('B', 'rel', 'a', 'b');
            """)

    async def test_to_type(self, client: Client):
        # bug #473
        await client.query("""//ti
            set_type('T', {
                ver: 'pint(ro)'
            });
            .to_type('T');
        """)

        client2 = await get_client(self.node1)
        client2.set_default_scope('//stuff')

        ver, tp = await client.query("""//ti
            [.ver, type(root())];
        """)
        self.assertEqual(ver, 1)
        self.assertEqual(tp, "T")
        with self.assertRaisesRegex(
                OperationError,
                r"mismatch in type `T`; property `ver` is read-only"):
            await client.query("""//ti
                .ver = 3;
            """)

        ver2, tp2 = await client2.query("""//ti
            [.ver, type(root())];
        """)
        self.assertEqual(ver2, 1)
        self.assertEqual(tp2, "T")
        with self.assertRaisesRegex(
                OperationError,
                r"mismatch in type `T`; property `ver` is read-only"):
            await client.query("""//ti
                .ver = 3;
            """)


if __name__ == '__main__':
    run_test(TestReadOnly())

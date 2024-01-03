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


dump = None


class TestNested(TestBase):

    title = 'Test nested and more complex queries'

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

    async def test_export(self, client0, client1):
        global dump
        await client0.query(r"""//ti
            .x = 42;
            .t = task(datetime(), |t| {
                log('running task...');
                t.again_in('seconds', 1);
            });
            new_procedure('test', ||nil);
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
            await client0.query(r"""//ti
                wse();
                assert(.x == 42);
                assert(is_nil(.t.id()));
                assert(procedures_info().len() == 1);
            """)

    async def test_import_tasks(self, client0, client1):
        await client0.query(r"""//ti
            import(dump, {import_tasks: true});
        """, dump=dump)

        for client in (client0, client1):
            await client0.query(r"""//ti
                wse();
                assert(.x == 42);
                assert(is_int(.t.id()));
                assert(procedures_info().len() == 1);
            """)

if __name__ == '__main__':
    run_test(TestNested())

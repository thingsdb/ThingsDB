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

    async def test_restriction_type(self, client):
        await client.query(r"""//ti
            set_type('X', {lookup: 'thing<float>'});
            .x = X{};
            .x.lookup.pi = 3.14;
        """)

        with self.assertRaisesRegex(TypeError, "restriction mismatch"):
            await client.query('.x.lookup.name = "test";')

        res = await client.query(r"""//ti
            mod_type(
                'X',
                'mod',
                'lookup',
                'thing<int>',
                |x| {pi: int(x.lookup.pi)});
            assert (.x.lookup.len() == 1);
            assert (.x.lookup.pi == 3);

            mod_type(
                'X',
                'mod',
                'lookup',
                'thing<X>',
                || thing());

            assert (.x.lookup.len() == 0);

            'OK';
        """)
        self.assertEqual(res, 'OK')


if __name__ == '__main__':
    run_test(TestRestriction())

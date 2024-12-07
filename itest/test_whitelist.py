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


class TestWhitelist(TestBase):

    title = 'Test whitelist'

    @default_test_setup(num_nodes=2, seed=1, threshold_full_storage=10)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('/t')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_whitelist(self, client):
        q = client.query
        r = client.run
        user = await q('user_info().load().name;')
        res = await q("""//ti
            new_procedure('sum', |a, b| a + b);
            new_procedure('mul', |a, b| a * b);
            whitelist_add(user, "procedures");
        """, user=user)

        res = await r('sum', 3, 4)
        self.assertEqual(res, 7)

        res = await r('mul', 3, 4)





if __name__ == '__main__':
    run_test(TestWhitelist())

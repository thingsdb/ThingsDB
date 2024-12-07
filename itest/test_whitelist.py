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
        client.set_default_scope('//stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_whitelist(self, client):
        q = client.query
        r = client.run
        await q("""//ti
            user = user_info().load().name;
            whitelist_add(user, "procedures", /^math_.*/);
            whitelist_add(user, "procedures", "sum");
            whitelist_add(user, "rooms", "app");
            whitelist_add(user, "rooms", /^str_.*/);
        """, scope='/t')

        room_ids = await q("""//ti
            new_procedure('sum', |a, b| a + b);
            new_procedure('mul', |a, b| a * b);
            new_procedure('math_sum', |a, b| a + b);
            new_procedure('math_mul', |a, b| a * b);
            rval = [
                .app_room = room(),
                .web_room = room(),
                .no_name_room = room(),
                .str_room_lower = room(),
                .str_room_upper = room(),
            ];
            .app_room.set_name('app');
            .web_room.set_name('web');
            .str_room_lower.set_name('str_lower');
            .str_room_upper.set_name('str_upper');
            rval.map_id();
        """)

        res = await r('sum', 3, 4)
        self.assertEqual(res, 7)

        res = await r('math_mul', 3, 4)
        self.assertEqual(res, 12)

        res = await client._join(room_ids[2])




if __name__ == '__main__':
    run_test(TestWhitelist())

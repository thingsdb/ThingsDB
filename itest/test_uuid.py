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


class TestUuid(TestBase):

    title = 'Test UUID type'

    @default_test_setup(num_nodes=1, seed=1, threshold_full_storage=10)
    async def async_run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_uuid(self, client):
        u = await client.query("""//ti
            uuid();
        """)
        self.assertIsInstance(u, str)

        u = await client.query("""//ti
            uuid(base64_decode("AaCLQ0q9dSmCoprjUrLxDw=="));
        """)
        self.assertEqual(u, '01a08b43-4abd-7529-82a2-9ae352b2f10f');


if __name__ == '__main__':
    run_test(TestUuid())

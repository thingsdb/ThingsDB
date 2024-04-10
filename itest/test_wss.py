#!/usr/bin/env python
import asyncio
import pickle
import time
import msgpack
import ssl
import struct
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


class TestWSS(TestBase):

    title = 'Test WebSocket (Secure TLS/SSL)'

    @default_test_setup(num_nodes=3, seed=1, threshold_full_storage=10,
                        enable_wss=True)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_simple_wss(self, client):
        self.assertTrue(client.is_websocket())
        res = await client.query('6 * 7;')
        self.assertEqual(res, 42)


if __name__ == '__main__':
    run_test(TestWSS())

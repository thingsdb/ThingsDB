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
from websockets.sync.client import connect


st_package = struct.Struct('<IHBB')
context = ssl._create_unverified_context()


def wss_example(port: int, use_ssl: bool = False):
    ctx = context if use_ssl else None
    proto = 'wss://' if use_ssl else 'ws://'
    with connect(f"{proto}localhost:{port}", ssl_context=ctx) as websocket:
        data = msgpack.packb(['admin', 'pass'])
        header = st_package.pack(
            len(data),
            0,
            33,
            33 ^ 0xff)
        websocket.send(header+data)
        resp = websocket.recv()
        data = msgpack.packb(['//stuff', """//ti
            6 * 7;
        """])
        header = st_package.pack(
            len(data),
            0,
            34,
            34 ^ 0xff)
        websocket.send(header+data)
        resp = websocket.recv()
        answer = msgpack.unpackb(resp[8:])
        return answer


class TestWSS(TestBase):

    title = 'Test WebSockets (Secure TLS/SSl)'

    @default_test_setup(num_nodes=3, seed=1, threshold_full_storage=10,
                        enable_wss=True)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_simple_ws(self, client):
        res = wss_example(self.node0.ws_port, use_ssl=True)
        self.assertEqual(res, 42)
        await asyncio.sleep(0.5)


if __name__ == '__main__':
    run_test(TestWSS())

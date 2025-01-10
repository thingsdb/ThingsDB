#!/usr/bin/env python
import asyncio
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client
from thingsdb.client import Client
from thingsdb.client import protocol


async def test_err_max_size():
    client = Client()

    # We decrease the max_size so the following query will fail
    protocol.WEBSOCKET_MAX_SIZE = 2**14

    await client.connect('ws://localhost:9780')
    try:
        await client.authenticate('admin', 'pass')

        # This is large enough so it will fail
        n = 20_000
        await client.query("""//ti
            range(n).map(|i| `this is item with number {i}`);
        """, timeout=5, n=n)

    finally:
        await client.close_and_wait()


class TestWS(TestBase):

    title = 'Test WebSockets'

    @default_test_setup(num_nodes=3, seed=1, threshold_full_storage=10,
                        enable_ws=True)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()
        await asyncio.sleep(1)  # sleep is required for nice close

    async def test_simple_ws(self, client: Client):
        self.assertTrue(client.is_websocket())
        info = client.connection_info()
        self.assertIsInstance(info, str)
        res = await client.query('6 * 7;')
        self.assertEqual(res, 42)

        # test with exact module frame size
        n = 10_156
        res = await client.query("""//ti
            [range(n).map(|i| 'aa')];
        """, n=n)
        self.assertEqual(len(res[0]), n)

        # test with large data
        n = 100_000
        res = await client.query("""//ti
            range(n).map(|i| `this is item with number {i}`);
        """, n=n)
        self.assertEqual(len(res), n)

    async def test_with_error(self, _client: Client):
        with self.assertRaises(asyncio.TimeoutError):
            await test_err_max_size()


if __name__ == '__main__':
    run_test(TestWS())

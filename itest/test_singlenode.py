#!/usr/bin/env python
import asyncio
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client


class TestMultiNode(TestBase):

    @default_test_setup(num_nodes=1, seed=1)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)

        await client.watch()

        await client.query(r'''
            new_collection("stuff");
        ''')

        await client.query(r'''
            counter = 0;
        ''', target='stuff')

        await self.node0.stop()

        client.close()
        await client.wait_closed()


if __name__ == '__main__':
    run_test(TestMultiNode())

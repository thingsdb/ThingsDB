#!/usr/bin/env python
import asyncio
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client


class TestMultiNode(TestBase):

    @default_test_setup(3)
    async def run(self):

        secret = '123bla'

        await self.node0.init_and_run()

        client = await get_client(self.node0)

        await client.query(r'''
            new_collection("stuff");
        ''')

        await self.node1.join(client)
        await self.node2.join(client, attempts=30)

        await asyncio.sleep(30)

        client.close()
        await client.wait_closed()


if __name__ == '__main__':
    run_test(TestMultiNode())

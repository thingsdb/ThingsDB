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
        await self.node1.join(secret)
        await self.node2.join(secret)

        client = await get_client(self.node0)

        await client.query(r'''
            new_collection("stuff");
        ''')

        await client.query(f'''
            new_node("{secret}", "127.0.0.1", {self.node1.listen_node_port});
        ''')

        await asyncio.sleep(30)

        await client.query(f'''
            new_node("{secret}", "127.0.0.1", {self.node2.listen_node_port});
        ''')

        await asyncio.sleep(90)

        client.close()
        await client.wait_closed()


if __name__ == '__main__':
    run_test(TestMultiNode())

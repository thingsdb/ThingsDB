#!/usr/bin/env python
import asyncio
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client


class TestMultiNode(TestBase):

    @default_test_setup(4)
    async def run(self):

        secret = '123bla'

        await self.node0.init_and_run()

        client = await get_client(self.node0)

        await client.query(r'''
            new_collection("stuff");
        ''')

        await client.query(r'''
            x = 1;
            y = 2;
            user = {
                name: 'Iris',
                age: 6,
            };
        ''', target='stuff')

        await self.node1.join(client)
        await asyncio.sleep(30)
        await self.node2.join(client)
        await self.node3.join(client)

        for _ in range(30):
            try:
                await client.query(r'''
                    x += 1;
                ''', target='stuff')
            except Exception:
                pass
            await asyncio.sleep(1)

        await asyncio.sleep(300)

        client.close()
        await client.wait_closed()


if __name__ == '__main__':
    run_test(TestMultiNode())

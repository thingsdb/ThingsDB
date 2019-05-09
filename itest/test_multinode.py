#!/usr/bin/env python
import asyncio
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client


class TestMultiNode(TestBase):

    @default_test_setup(5)
    async def run(self):

        secret = '123bla'

        await self.node0.init_and_run()

        client = await get_client(self.node0)

        await client.query(r'''
            new_collection("stuff");
        ''')

        await client.query(r'''
            counter = 0;
        ''', target='stuff')

        await self.node1.join_until_ready(client)
        await self.node2.join_until_ready(client)
        await self.node3.join_until_ready(client)
        await self.node4.join_until_ready(client)

        client.close()
        client = await get_client(
            self.node0,
            self.node1,
            self.node2,
            self.node3,
            self.node4)

        for _ in range(30):
            await client.query(r'''
                counter += 1;
            ''', target='stuff')

        await asyncio.sleep(0.5)

        for node in (self.node0, self.node1, self.node2, self.node3):
            fut = node.stop()

            await asyncio.sleep(0.1)

            counter = await client.query(r'counter;', target='stuff')
            assert (counter == 30)

            await fut

            await asyncio.sleep(1.0)

            counter = await client.query(r'counter;', target='stuff')
            assert (counter == 30)

        client.close()
        await client.wait_closed()

        print('!!!!!\n\n  CLOSED! \n\n !!!!!!!!!!')


if __name__ == '__main__':
    run_test(TestMultiNode())

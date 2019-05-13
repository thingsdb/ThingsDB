#!/usr/bin/env python
import asyncio
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client


class TestMultiNode(TestBase):

    title = 'Test multi node client connection'

    @default_test_setup(num_nodes=5, seed=2)
    async def run(self):

        expected_counter = 10

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

        for _ in range(expected_counter):
            await client.query(r'''
                counter += 1;
            ''', target='stuff')

        for node in (self.node0, self.node1, self.node2, self.node3):
            # A soft kill and we do not wait for the result. The goal here is
            # to test if all the queries which follow are still returning as
            # expected.
            node.soft_kill()

            # Test a few times for a correct counter response, the client
            # will loose a connection and should reconnect while no
            # queries get lost.
            for _ in range(20):
                counter = await client.query(r'counter;', target='stuff')
                assert (counter == expected_counter)
                await asyncio.sleep(0.2)

        client.close()
        await client.wait_closed()


if __name__ == '__main__':
    run_test(TestMultiNode())

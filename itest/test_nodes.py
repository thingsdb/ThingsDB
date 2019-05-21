#!/usr/bin/env python
import asyncio
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client
from thingsdb.target import Target
from thingsdb.exceptions import NodeError


class TestNodes(TestBase):

    title = 'Test add, pop and replace nodes'

    @default_test_setup(num_nodes=5, seed=2)
    async def run(self):
        await self.node0.init_and_run()

        client = await get_client(self.node0)
        stuff = Target('stuff')

        await self.node1.join_until_ready(client)
        await self.node2.join_until_ready(client)

        self.assertEqual(
            len(await client.query(r'nodes();', target=client.node)), 3)

        with self.assertRaisesRegex(
                NodeError,
                r'`node:2` is still active, '
                r'shutdown the node before removal'):
            await client.query(r'pop_node();')

        await self.node2.shutdown()
        await client.query(r'pop_node();')

        self.assertEqual(
            len(await client.query(r'nodes();', target=client.node)), 2)

        await self.node3.join_until_ready(client)

        self.assertEqual(
            len(await client.query(r'nodes();', target=client.node)), 3)

        await self.node4.wait_join(secret='letsgo')

        with self.assertRaisesRegex(
                NodeError,
                r'`node:1` is still active, '
                r'shutdown the node before replacement'):
            await client.query('replace_node(1, "letsgo", "127.0.0.1", 9224);')

        await self.node1.shutdown()
        await client.query('replace_node(1, "letsgo", "127.0.0.1", 9224);')

        nodes = await client.query(r'nodes();', target=client.node)
        self.assertEqual(len(nodes), 3)

        await client.query('hello = "world";', target=stuff)

        await asyncio.sleep(50)  # 50 seconds should be enough to sync

        cl3 = await get_client(self.node3)  # node:2
        cl4 = await get_client(self.node4)  # node:1

        self.assertEqual((await cl3.query('hello;', target=stuff)), 'world')
        self.assertEqual((await cl4.query('hello;', target=stuff)), 'world')

        cl3.close()
        cl4.close()
        client.close()
        await cl3.wait_closed()
        await cl4.wait_closed()
        await client.wait_closed()

        await self.node0.shutdown()
        await self.node3.shutdown()
        await self.node4.shutdown()

        await self.node0.run()
        await self.node3.run()
        await self.node4.run()

        await asyncio.sleep(50)  # 50 seconds should be enough to sync

        client = await get_client(self.node0)
        nodes = await client.query(r'nodes();', target=client.node)
        for node in nodes:
            self.assertEqual(node['commited_event_id'], 9)

        client.close()
        await client.wait_closed()


if __name__ == '__main__':
    run_test(TestNodes())

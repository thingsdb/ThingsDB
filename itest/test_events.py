#!/usr/bin/env python
import asyncio
import pickle
import time
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


class TestEvents(TestBase):

    title = 'Test with multiple events at the same time'

    @default_test_setup(num_nodes=3, seed=1, threshold_full_storage=10)
    async def run(self):
        x = 20

        await self.node0.init_and_run()

        client0 = await get_client(self.node0)
        client0.use('stuff')

        await client0.query('.x = 0;')

        await self.node1.join_until_ready(client0)

        client1 = await get_client(self.node1)
        client1.use('stuff')

        for _ in range(x):
            await self.mquery('.x += 1', client0, client1)

        i = 0
        while True:
            nodes_info = await client0.query('nodes_info();', scope='@n')
            if all([node['status'] == 'READY' for node in nodes_info]):
                await self.mquery('.x += 1', client0, client1)
                i += 1
                if i == x:
                    break
                continue

            await asyncio.sleep(0.5)

        await self.node2.join_until_ready(client0)

        client2 = await get_client(self.node2)
        client2.use('stuff')

        for _ in range(x):
            await self.mquery('.x += 1', client0, client1, client2)

        await asyncio.sleep(0.2)

        for client in (client0, client1, client2):
            self.assertEqual(await client.query('.x'), x * 7)

        i = 0
        while True:
            nodes_info = await client0.query('nodes_info();', scope='@n')
            if all([node['status'] == 'READY' for node in nodes_info]):
                await self.mquery('.x += 1', client0, client1, client2)
                i += 1
                if i == x:
                    break
                continue

            await asyncio.sleep(0.5)

        await asyncio.sleep(0.2)

        # check so that we are sure none of the nodes is in away mode
        checked = False
        while True:
            nodes_info = await client0.query('nodes_info();', scope='@n')
            if all([node['status'] == 'READY' for node in nodes_info]):
                if checked:
                    break
                for client in (client0, client1, client2):
                    self.assertEqual(await client.query('.x'), x * 10)
                checked = True

            await asyncio.sleep(0.5)

        # expected no garbage collection
        for client in (client0, client1, client2):
            counters = await client.query('counters();', scope='@node')
            self.assertEqual(counters['garbage_collected'], 0)
            self.assertEqual(counters['events_failed'], 0)

            client.close()
            await client.wait_closed()

    async def mquery(self, query, *clients):
        tasks = (client.query(query) for client in clients)
        await asyncio.gather(*tasks)


if __name__ == '__main__':
    run_test(TestEvents())

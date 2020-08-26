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

    @default_test_setup(num_nodes=4, seed=1, threshold_full_storage=1000000)
    async def run(self):
        x = 30
        mq = '.x += .x % {}; .arr.push(.x);'

        await self.node0.init_and_run()

        client0 = await get_client(self.node0)
        client0.set_default_scope('//stuff')
        client0.id = 2

        await client0.query('.x = 10; .arr = [];')

        await self.node1.join_until_ready(client0)

        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')
        client1.id = 5

        for _ in range(x):
            await self.mquery(mq, client0, client1)

        i = 0
        while True:
            nodes_info = await client0.query('nodes_info();', scope='@n')
            if all([node['status'] == 'READY' for node in nodes_info]):
                await self.mquery(mq, client0, client1)
                i += 1
                if i == x:
                    break
                continue

            await asyncio.sleep(0.5)

        await self.node2.join_until_ready(client0)

        client2 = await get_client(self.node2)
        client2.set_default_scope('//stuff')
        client2.id = 9

        # Added sleep() to test if this prevents a timeout-error which
        # may sometimes happen on the `mquery` below
        await asyncio.sleep(0.2)

        for _ in range(x):
            await self.mquery(mq, client0, client1, client2)

        await asyncio.sleep(0.2)

        for client in (client0, client1, client2):
            self.assertEqual(await client.query('.arr.len()'), x * 7)

        i = 0
        while True:
            nodes_info = await client0.query('nodes_info();', scope='@n')
            if all([node['status'] == 'READY' for node in nodes_info]):
                await self.mquery(mq, client0, client1, client2)
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
                    self.assertEqual(await client.query('.arr.len()'), x * 10)
                checked = True

            await asyncio.sleep(0.5)

        await asyncio.gather(
            self.node3.join_until_ready(client0),
            self.loop_add(x * 3, mq, client0, client1, client2))

        client3 = await get_client(self.node3)
        client3.set_default_scope('//stuff')
        client3.id = 11

        await asyncio.sleep(0.5)
        check_arr = await client0.query('.arr;')

        # `arr` might be different in each run, but must be equal on all nodes

        checked = False
        while True:
            nodes_info = await client0.query('nodes_info();', scope='@n')
            if all([node['status'] == 'READY' for node in nodes_info]):
                if checked:
                    break
                for client in (client0, client1, client2, client3):
                    self.assertEqual(await client.query('.arr.len();'), x * 19)
                    self.assertEqual(await client.query('.arr;'), check_arr)
                checked = True

            await asyncio.sleep(0.5)

        await self.close_client(client0)
        self.node0.soft_kill()

        for _ in range(x):
            await self.mquery(mq, client1, client2, client3)

        for _ in range(x):
            await self.mquery(mq, client1, client2, client3)
            await asyncio.sleep(0.4)

        await asyncio.sleep(0.5)
        check_arr = await client1.query('.arr;')

        # `arr` might be different in each run, but must be equal on all nodes

        checked = False
        while True:
            nodes_info = await client1.query('nodes_info();', scope='@n')

            if all([node['status'] == 'READY'
                    for node in nodes_info
                    if node['node_id'] != 0]):
                if checked:
                    break
                for client in (client1, client2, client3):
                    self.assertEqual(await client.query('.arr.len();'), x * 25)
                    self.assertEqual(await client.query('.arr;'), check_arr)
                checked = True

            await asyncio.sleep(0.5)

        # expected no garbage collection
        for client in (client1, client2, client3):
            await self.close_client(client)

    async def close_client(self, client):
        counters = await client.query('counters();', scope='@node')
        self.assertEqual(counters['garbage_collected'], 0)
        self.assertEqual(counters['events_failed'], 0)

        client.close()
        await client.wait_closed()

    async def mquery(self, query, *clients):
        tasks = (client.query(
            query.format(client.id), timeout=10) for client in clients)
        await asyncio.gather(*tasks)

    async def loop_add(self, x, q, *clients):
        for _ in range(x):
            await self.mquery(q, *clients)
            await asyncio.sleep(0.1)


if __name__ == '__main__':
    run_test(TestEvents())

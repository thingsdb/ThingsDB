#!/usr/bin/env python
"""
Start this test using:

THINGSDB_BIN=../thingsdb THINGSDB_KEEP_ON_ERROR=1 THINGSDB_VERBOSE=1 THINGSDB_MEMCHECK=1 THINGSDB_NODE_OUTPUT= THINGSDB_LOGLEVEL=debug ./test_pre_restore.py

Example restore:

restore('/home/joente/Downloads/playground_20200903040003.tar.gz', {take_access: true});
"""  # nopep8
import os
import asyncio
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client
from thingsdb.exceptions import NodeError
from lib.vars import THINGSDB_KEEP_ON_ERROR


NUM_NODES = int(os.getenv("NUM_NODES", 2))

assert 1 <= NUM_NODES <= 3


class TestNodes(TestBase):

    title = 'Test prepare for restore'

    @default_test_setup(num_nodes=NUM_NODES, seed=2)
    async def run(self):
        await self.node0.init_and_run()

        client = await get_client(self.node0)
        await client.del_collection('stuff')

        if NUM_NODES > 1:
            await self.node1.join_until_ready(client)

        if NUM_NODES > 2:
            await self.node2.join_until_ready(client)

        client.close()
        await client.wait_closed()
        if THINGSDB_KEEP_ON_ERROR:
            assert 0, 'Intential assertion to keep ThingsDB Running'


if __name__ == '__main__':
    run_test(TestNodes())

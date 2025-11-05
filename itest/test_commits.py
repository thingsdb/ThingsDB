#!/usr/bin/env python
import asyncio
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client
from thingsdb.exceptions import TypeError
from thingsdb.exceptions import NumArgumentsError
from thingsdb.exceptions import LookupError
from thingsdb.exceptions import OperationError


class TestCommits(TestBase):

    title = 'Test commit history'

    def with_node1(self):
        if hasattr(self, 'node1'):
            return True
        print('''
            WARNING: Test requires a second node!!!
        ''')

    @default_test_setup(num_nodes=1, seed=1, threshold_full_storage=10)
    async def async_run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        # add another node
        # if hasattr(self, 'node1'):
        #     await self.node1.join_until_ready(client)

        await self.run_tests(client)

        client.close()
        await client.wait_closed()


if __name__ == '__main__':
    run_test(TestCommits())

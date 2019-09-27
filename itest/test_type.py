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


class TestTypes(TestBase):

    title = 'Test thingsdb types'

    @default_test_setup(num_nodes=1, seed=1, threshold_full_storage=100)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.use('stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_new_type(self, client):
        await client.query(r'''
            new_type('User', {
                name: 'str',
                age: 'uint',
                likes: '[]?',
            });
            new_type('People', {
                users: '[User]'
            });
        ''')

        await client.query(r'''
            .iris = new('User', {
                name: 'Iris',
                age: 6
            });
            .people = new('People', {users: [.iris]});
        ''')


if __name__ == '__main__':
    run_test(TestTypes())

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
from thingsdb.exceptions import ForbiddenError


class TestModules(TestBase):

    title = 'Test modules'

    @default_test_setup(
        num_nodes=1,
        seed=1,
        threshold_full_storage=10,
        modules_path='../modules/demo')
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_new_modules(self, client):
        await client.query(r'''
            new_module('DEMO', 'demo', nil, nil);
        ''', scope='/t')

        res = await client.query(r'''
            future({
                module: 'DEMO',
                message: 'Hi ThingsDB!'
            }).then(|msg| {
                `Got this message back: {msg}`
            });
        ''')
        print(res)


if __name__ == '__main__':
    run_test(TestModules())

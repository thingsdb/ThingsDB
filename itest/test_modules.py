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
        modules_path='../modules/')
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_demo_module(self, client):
        await client.query(r'''
            new_module('DEMO', 'demo/demo', nil, nil);
        ''', scope='/t')

        res = await client.query(r'''
            future({
                module: 'DEMO',
                message: 'Hi ThingsDB!'
            }).then(|msg| {
                `Got this message back: {msg}`
            });
        ''')
        self.assertEqual(res, 'Got this message back: Hi ThingsDB!')

    async def test_requests_module(self, client):
        await client.query(r'''
            new_module('REQUESTS', 'requests/requests', nil, nil);
        ''', scope='/t')

        res = await client.query(r'''
            future({
                module: 'REQUESTS',
                url: 'http://localhost:8080/status'
            }).then(|resp| [str(resp.body).trim(), resp.status_code]);
        ''')
        self.assertEqual(res, ['READY', 200])

    async def test_siridb_module(self, client):
        await client.query(r'''
            new_module('SIRIDB', 'siridb/siridb', {
                username: 'iris',
                password: 'siri',
                database: 'dbtest',
                servers: [{
                    host: 'localhost',
                    port: -100
                }]
            }, nil);
        ''', scope='/t')

        await asyncio.sleep(3)

        res = await client.query(r'''
            future({
                module: 'SIRIDB',
                type: 'QUERY',
                query: 'select * from *'
            });
        ''')
        print(res)


if __name__ == '__main__':
    run_test(TestModules())

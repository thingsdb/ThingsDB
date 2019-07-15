#!/usr/bin/env python
import asyncio
import pickle
import time
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client
from thingsdb.exceptions import AssertionError
from thingsdb.exceptions import BadRequestError
from thingsdb.exceptions import IndexError
from thingsdb.exceptions import OverflowError
from thingsdb.exceptions import ZeroDivisionError


class TestProcedureFunctions(TestBase):

    title = 'Test procedure functions'

    @default_test_setup(num_nodes=1, seed=1)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_new_procedure(self, client):
        with self.assertRaisesRegex(
                BadRequestError,
                'function `new_procedure` takes 1 argument '
                'but 0 were given'):
            await client.query('new_procedure();')

        with self.assertRaisesRegex(
                BadRequestError,
                r'function `new_procedure` expects argument 1 to be of '
                r'type `raw` but got type `nil` instead'):
            await client.query('new_procedure(nil);')

        client.use('stuff')
        with self.assertRaisesRegex(
                BadRequestError,
                r'function `new_procedure` expects a definition to have '
                r'valid UTF8 encoding'):
            await client.query(
                'new_procedure(blob(0));',
                blobs=[pickle.dumps('binary')])


if __name__ == '__main__':
    run_test(TestProcedureFunctions())

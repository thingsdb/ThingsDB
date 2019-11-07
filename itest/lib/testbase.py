import unittest
import asyncio
from thingsdb.exceptions import NodeError
from .client import get_client


class TestBase(unittest.TestCase):

    title = 'no test title'

    async def run(self):
        raise NotImplementedError('run must be implemented')

    async def wait_nodes_ready(self, client=None):
        if client is None:
            client = await get_client(self.node0)

        attempts = 20
        while attempts:
            try:
                res = await client.nodes_info()
            except NodeError:
                pass
            else:
                if all((node['status'] == 'READY' for node in res)):
                    return
            attempts -= 1
            await asyncio.sleep(0.5)

    async def run_tests(self, *args, **kwargs):
        for attr, f in self.__class__.__dict__.items():
            if attr.startswith('test_') and callable(f):
                client = await get_client(self.node0)
                await client.query(r'''
                    del_collection('stuff');
                    new_collection('stuff');
                ''')
                await self.wait_nodes_ready(client)
                await f(self, *args, **kwargs)
                client.close()
                await client.wait_closed()

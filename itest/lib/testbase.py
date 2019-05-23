import unittest
from .client import get_client


class TestBase(unittest.TestCase):

    title = 'no test title'

    async def run(self):
        raise NotImplementedError('run must be implemented')

    async def run_tests(self, *args, **kwargs):
        for attr, f in self.__class__.__dict__.items():
            if attr.startswith('test_') and callable(f):
                client = await get_client(self.node0)
                await client.query(r'''
                    del_collection('stuff');
                    new_collection('stuff');
                ''')
                await f(self, *args, **kwargs)
                client.close()
                await client.wait_closed()

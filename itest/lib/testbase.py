import unittest
import asyncio
from thingsdb.exceptions import NodeError
from .client import get_client


class TestBase(unittest.TestCase):

    title = 'no test title'

    async def run(self):
        raise NotImplementedError('run must be implemented')

    async def wait_nodes_ready(self, client=None, success_count=10):
        if client is None:
            client = await get_client(self.node0)

        count = 0
        attempts = 120  # at most 2 minutes
        while attempts:
            try:
                res = await client.nodes_info()
            except NodeError:
                pass
            else:
                if all((node['status'] == 'READY' for node in res)):
                    count += 1
                    if count >= success_count:  # at least X times successful
                        return
                else:
                    success = 0
            attempts -= 1
            await asyncio.sleep(0.5)

    async def wait_nodes_stored(self, client=None):
        if client is None:
            client = await get_client(self.node0)

        attempts = 120  # at most 2 minutes
        while attempts:
            try:
                res = await client.nodes_info()
            except NodeError:
                pass
            else:
                event_id = res[0]['committed_change_id']
                if all((
                    node['stored_change_id'] == event_id
                    for node in res)) and all((
                        node['committed_change_id'] == event_id
                        for node in res)):
                    return  # success
            attempts -= 1
            await asyncio.sleep(0.5)

    async def assertChange(self, client, query):
        before = \
            (await client.query('counters()', scope='@n'))['changes_committed']
        await client.query(query)
        after = \
            (await client.query('counters()', scope='@n'))['changes_committed']
        self.assertGreater(
            after-before,
            0,
            f'missing event for query: `{query}`')

    async def assertNoChange(self, client, query):
        before = \
            (await client.query('counters()', scope='@n'))['changes_committed']
        await client.query(query)
        after = \
            (await client.query('counters()', scope='@n'))['changes_committed']
        self.assertEqual(
            before,
            after,
            f'unexpected event for query: `{query}`')

    async def run_tests(self, *args, **kwargs):
        for attr, f in self.__class__.__dict__.items():
            if attr.startswith('test_') and callable(f):
                client = await get_client(self.node0)
                await client.query(r'''
                    del_collection('stuff');
                    new_collection('stuff');
                ''')
                await self.wait_nodes_ready(client, success_count=1)
                await f(self, *args, **kwargs)
                client.close()
                await client.wait_closed()

    async def wait_for_module(self, client, module, timeout=30):
        while timeout:
            timeout -= 1
            status = await client.query(
                'module_info(module).load().status;',
                module=module)
            if status == 'running':
                break
            await asyncio.sleep(1)

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


class TestNodeFunctions(TestBase):

    title = 'Test node scope functions'

    @default_test_setup(num_nodes=1, seed=1)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.use(client.node)

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_counters(self, client):
        with self.assertRaisesRegex(
                BadRequestError,
                'function `counters` takes 0 arguments but 1 was given'):
            await client.query('counters("Test!");')

        counters = await client.query('counters();')

        self.assertEqual(len(counters), 11)

        self.assertIn("queries_received", counters)
        self.assertIn("events_with_gap", counters)
        self.assertIn("events_skipped", counters)
        self.assertIn("events_failed", counters)
        self.assertIn("events_killed", counters)
        self.assertIn("events_committed", counters)
        self.assertIn("events_quorum_lost", counters)
        self.assertIn("events_unaligned", counters)
        self.assertIn("garbage_collected", counters)
        self.assertIn("longest_event_duration", counters)
        self.assertIn("average_event_duration", counters)

        self.assertTrue(isinstance(counters["queries_received"], int))
        self.assertTrue(isinstance(counters["events_with_gap"], int))
        self.assertTrue(isinstance(counters["events_skipped"], int))
        self.assertTrue(isinstance(counters["events_failed"], int))
        self.assertTrue(isinstance(counters["events_killed"], int))
        self.assertTrue(isinstance(counters["events_committed"], int))
        self.assertTrue(isinstance(counters["events_quorum_lost"], int))
        self.assertTrue(isinstance(counters["events_unaligned"], int))
        self.assertTrue(isinstance(counters["garbage_collected"], int))
        self.assertTrue(isinstance(counters["longest_event_duration"], float))
        self.assertTrue(isinstance(counters["average_event_duration"], float))


if __name__ == '__main__':
    run_test(TestNodeFunctions())

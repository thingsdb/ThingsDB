#!/usr/bin/env python
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client
from thingsdb.exceptions import TypeError
from thingsdb.exceptions import NumArgumentsError


class TestNodeFunctions(TestBase):

    title = 'Test node scope functions'

    @default_test_setup(num_nodes=1, seed=1)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('@n')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_counters(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `counters` takes 0 arguments but 1 was given'):
            await client.query('counters("Test!");')

        counters = await client.query('counters();')

        self.assertEqual(len(counters), 20)

        self.assertIn("average_change_duration", counters)
        self.assertIn("average_query_duration", counters)
        self.assertIn("changes_committed", counters)
        self.assertIn("changes_failed", counters)
        self.assertIn("changes_killed", counters)
        self.assertIn("changes_skipped", counters)
        self.assertIn("changes_unaligned", counters)
        self.assertIn("changes_with_gap", counters)
        self.assertIn("garbage_collected", counters)
        self.assertIn("largest_result_size", counters)
        self.assertIn("longest_change_duration", counters)
        self.assertIn("longest_query_duration", counters)
        self.assertIn("queries_from_cache", counters)
        self.assertIn("queries_success", counters)
        self.assertIn("queries_with_error", counters)
        self.assertIn("quorum_lost", counters)
        self.assertIn("started_at", counters)
        self.assertIn("tasks_success", counters)
        self.assertIn("tasks_with_error", counters)
        self.assertIn("wasted_cache", counters)

        self.assertTrue(isinstance(counters["average_change_duration"], float))
        self.assertTrue(isinstance(counters["average_query_duration"], float))
        self.assertTrue(isinstance(counters["changes_committed"], int))
        self.assertTrue(isinstance(counters["changes_failed"], int))
        self.assertTrue(isinstance(counters["changes_killed"], int))
        self.assertTrue(isinstance(counters["changes_skipped"], int))
        self.assertTrue(isinstance(counters["changes_unaligned"], int))
        self.assertTrue(isinstance(counters["changes_with_gap"], int))
        self.assertTrue(isinstance(counters["garbage_collected"], int))
        self.assertTrue(isinstance(counters["largest_result_size"], int))
        self.assertTrue(isinstance(counters["longest_change_duration"], float))
        self.assertTrue(isinstance(counters["longest_query_duration"], float))
        self.assertTrue(isinstance(counters["queries_from_cache"], int))
        self.assertTrue(isinstance(counters["queries_success"], int))
        self.assertTrue(isinstance(counters["queries_with_error"], int))
        self.assertTrue(isinstance(counters["quorum_lost"], int))
        self.assertTrue(isinstance(counters["started_at"], int))
        self.assertTrue(isinstance(counters["tasks_success"], int))
        self.assertTrue(isinstance(counters["tasks_with_error"], int))
        self.assertTrue(isinstance(counters["wasted_cache"], int))

    async def test_node_info(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `node_info` takes 0 arguments but 1 was given'):
            await client.query('node_info(nil);')

        node = await client.query('node_info();')

        self.assertEqual(len(node), 41)

        self.assertIn("node_id", node)
        self.assertIn("version", node)
        self.assertIn("syntax_version", node)
        self.assertIn("msgpack_version", node)
        self.assertIn("libcleri_version", node)
        self.assertIn("libuv_version", node)
        self.assertIn("libpcre2_version", node)
        self.assertIn("libwebsockets_version", node)
        self.assertIn("yajl_version", node)
        self.assertIn("status", node)
        self.assertIn("zone", node)
        self.assertIn("log_level", node)
        self.assertIn("node_name", node)
        self.assertIn("client_port", node)
        self.assertIn("node_port", node)
        self.assertIn("ip_support", node)
        self.assertIn("storage_path", node)
        self.assertIn("uptime", node)
        self.assertIn("changes_in_queue", node)
        self.assertIn("archived_in_memory", node)
        self.assertIn("archive_files", node)
        self.assertIn("local_stored_change_id", node)
        self.assertIn("local_committed_change_id", node)
        self.assertIn("global_stored_change_id", node)
        self.assertIn("global_committed_change_id", node)
        self.assertIn("db_stored_change_id", node)
        self.assertIn("next_change_id", node)
        self.assertIn("next_free_id", node)
        self.assertIn("cached_names", node)
        self.assertIn('http_status_port', node)
        self.assertIn('http_api_port', node)
        self.assertIn('scheduled_backups', node)
        self.assertIn('connected_clients', node)
        self.assertIn('result_size_limit', node)
        self.assertIn('cached_queries', node)
        self.assertIn('threshold_query_cache', node)
        self.assertIn('cache_expiration_time', node)
        self.assertIn('python_interpreter', node)
        self.assertIn('modules_path', node)
        self.assertIn('architecture', node)
        self.assertIn('platform', node)

        self.assertTrue(isinstance(node["node_id"], int))
        self.assertTrue(isinstance(node["version"], str))
        self.assertTrue(isinstance(node["syntax_version"], str))
        self.assertTrue(isinstance(node["msgpack_version"], str))
        self.assertTrue(isinstance(node["libcleri_version"], str))
        self.assertTrue(isinstance(node["libuv_version"], str))
        self.assertTrue(isinstance(node["libpcre2_version"], str))
        self.assertTrue(isinstance(node["libwebsockets_version"], str))
        self.assertTrue(isinstance(node["yajl_version"], str))
        self.assertTrue(isinstance(node["status"], str))
        self.assertTrue(isinstance(node["zone"], int))
        self.assertTrue(isinstance(node["log_level"], str))
        self.assertTrue(isinstance(node["node_name"], str))
        self.assertTrue(isinstance(node["client_port"], int))
        self.assertTrue(isinstance(node["node_port"], int))
        self.assertTrue(isinstance(node["ip_support"], str))
        self.assertTrue(isinstance(node["storage_path"], str))
        self.assertTrue(isinstance(node["uptime"], float))
        self.assertTrue(isinstance(node["changes_in_queue"], int))
        self.assertTrue(isinstance(node["archived_in_memory"], int))
        self.assertTrue(isinstance(node["archive_files"], int))
        self.assertTrue(isinstance(node["local_stored_change_id"], int))
        self.assertTrue(isinstance(node["local_committed_change_id"], int))
        self.assertTrue(isinstance(node["global_stored_change_id"], int))
        self.assertTrue(isinstance(node["global_committed_change_id"], int))
        self.assertTrue(isinstance(node["db_stored_change_id"], int))
        self.assertTrue(isinstance(node["next_change_id"], int))
        self.assertTrue(isinstance(node["next_free_id"], int))
        self.assertTrue(isinstance(node["cached_names"], int))
        self.assertTrue(isinstance(node["http_status_port"], (int, str)))
        self.assertTrue(isinstance(node["http_api_port"], (int, str)))
        self.assertTrue(isinstance(node["scheduled_backups"], int))
        self.assertTrue(isinstance(node["connected_clients"], int))
        self.assertTrue(isinstance(node["result_size_limit"], int))
        self.assertTrue(isinstance(node["cached_queries"], int))
        self.assertTrue(isinstance(node["threshold_query_cache"], int))
        self.assertTrue(isinstance(node["cache_expiration_time"], int))
        self.assertTrue(isinstance(node["python_interpreter"], str))
        self.assertTrue(isinstance(node["modules_path"], str))
        self.assertTrue(isinstance(node["architecture"], str))
        self.assertTrue(isinstance(node["platform"], str))

    async def test_nodes_info(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `nodes_info` takes 0 arguments but 1 was given'):
            await client.query('nodes_info(nil);')

        nodes = await client.query('nodes_info();')
        node = nodes.pop()

        self.assertEqual(len(node), 9)

        self.assertIn("node_name", node)
        self.assertIn("committed_change_id", node)
        self.assertIn("node_id", node)
        self.assertIn("port", node)
        self.assertIn("status", node)
        self.assertIn('stored_change_id', node)
        self.assertIn('syntax_version', node)
        self.assertIn('zone', node)
        self.assertIn('stream', node)

        self.assertTrue(isinstance(node["node_name"], str))
        self.assertTrue(isinstance(node["committed_change_id"], int))
        self.assertTrue(isinstance(node["node_id"], int))
        self.assertTrue(isinstance(node["port"], int))
        self.assertTrue(isinstance(node["status"], str))
        self.assertTrue(isinstance(node["stored_change_id"], int))
        self.assertTrue(isinstance(node["syntax_version"], str))
        self.assertTrue(isinstance(node["zone"], int))
        self.assertIs(node["stream"], None)

    async def test_reset_counters(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `reset_counters` takes 0 arguments but 1 was given'):
            await client.query('reset_counters(1);')

        counters = await client.query('counters();')
        self.assertGreater(counters["queries_with_error"], 0)
        self.assertIs(await client.query('reset_counters();'), None)
        counters = await client.query('counters();')
        self.assertEqual(counters["queries_with_error"], 0)

    async def test_set_log_level(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `set_log_level` takes 1 argument but 0 were given'):
            await client.query('set_log_level();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `set_log_level` expects argument 1 to be of '
                r'type `int` but got type `str` instead'):
            await client.query('set_log_level("DEBUG");')

        prev = (await client.node_info())['log_level']

        self.assertIs(await client.query('set_log_level(ERROR);'), None)
        self.assertEqual((await client.node_info())['log_level'], 'ERROR')
        self.assertIs(await client.query('set_log_level(0);'), None)
        self.assertEqual((await client.node_info())['log_level'], 'DEBUG')
        self.assertIs(await client.query(f'set_log_level({prev});'), None)
        self.assertEqual((await client.node_info())['log_level'], prev)


if __name__ == '__main__':
    run_test(TestNodeFunctions())

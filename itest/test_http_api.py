#!/usr/bin/env python
import requests
import msgpack
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client



class TestHTTPAPI(TestBase):

    title = 'Test HTTP API'

    @default_test_setup(num_nodes=2, seed=1, threshold_full_storage=10)
    async def run(self):

        await self.node0.init_and_run()

        client0 = await get_client(self.node0)
        client0.set_default_scope('//stuff')

        # add more nodes for watch validation
        await self.node1.join_until_ready(client0)

        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')

        api0 = f'http://localhost:{self.node0.http_api_port}'
        api1 = f'http://localhost:{self.node1.http_api_port}'

        data = {
            'type': 'query',
            'code': '''
                new_user("test");
                grant("@t", "test", QUERY|RUN);
                new_token("test");
            '''
        }

        x = requests.post(
            f'{api0}/thingsdb',
            data=msgpack.dumps(data),
            auth=('admin', 'pass'),
            headers={'Content-Type': 'application/msgpack'}
        )

        self.assertEqual(x.status_code, 200)
        token = msgpack.unpackb(x.content, raw=False)

        await self.run_tests(api0, api1, token)

        # expected no garbage collection
        for client in (client0, client1):
            counters = await client.query('counters();', scope='@node')
            self.assertEqual(counters['garbage_collected'], 0)
            self.assertEqual(counters['changes_failed'], 0)

            client.close()
            await client.wait_closed()

    async def test_msgpack_query(self, api0, api1, token):

        data = {
            'type': 'query',
            'code': '21 + 21;'
        }

        x = requests.post(
            f'{api0}//stuff',
            data=msgpack.dumps(data),
            auth=('admin', 'pass'),
            headers={'Content-Type': 'application/msgpack'}
        )

        self.assertEqual(x.status_code, 200)
        content = msgpack.unpackb(x.content, raw=False)

        self.assertEqual(content, 42)

        data = {
            'type': 'query',
            'code': 'node_info();'
        }

        x = requests.post(
            f'{api0}/n/1',
            data=msgpack.dumps(data),
            auth=('admin', 'pass'),
            headers={'Content-Type': 'application/msgpack'}
        )

        self.assertEqual(x.status_code, 200)
        content = msgpack.unpackb(x.content, raw=False)

        self.assertIn('version', content)
        self.assertIn('node_id', content)

        data = {
            'type': 'query',
            'code': '32 + x;',
            'vars': {
                'x': 10
            }
        }

        x = requests.post(
            f'{api0}//stuff',
            data=msgpack.dumps(data),
            auth=('admin', 'pass'),
            headers={'Content-Type': 'application/msgpack'}
        )

        self.assertEqual(x.status_code, 200)
        content = msgpack.unpackb(x.content, raw=False)

        self.assertEqual(content, 42)

    async def test_json_query(self, api0, api1, token):
        data = {
            'type': 'query',
            'code': '21 + 21;'
        }

        x = requests.post(
            f'{api0}//stuff',
            json=data,
            auth=('admin', 'pass'),
        )

        self.assertEqual(x.status_code, 200)
        content = x.json()

        self.assertEqual(content, 42)

        data = {
            'type': 'query',
            'code': 'node_info();'
        }

        x = requests.post(
            f'{api0}/n/1',
            json=data,
            auth=('admin', 'pass'),
        )

        self.assertEqual(x.status_code, 200)
        content = x.json()

        self.assertIn('version', content)
        self.assertIn('node_id', content)

        data = {
            'type': 'query',
            'code': '32 + x;',
            'vars': {'x': 10}
        }

        x = requests.post(
            f'{api0}//stuff',
            json=data,
            auth=('admin', 'pass'),
        )

        self.assertEqual(x.status_code, 200)
        content = x.json()

        self.assertEqual(content, 42)

    async def test_json_errors(self, api0, api1, token):
        x = requests.post(
            f'{api0}//unknown',
            json={'type': 'query', 'code': '42'},
            auth=('admin', 'pass'),
        )
        self.assertEqual(x.status_code, 404)
        self.assertEqual(x.text, 'collection `unknown` not found (-54)\r\n')

        x = requests.post(
            f'{api0}//stuff',
            data=b'123',
            auth=('admin', 'pass'),
        )
        self.assertEqual(x.status_code, 415)
        self.assertEqual(x.text, 'UNSUPPORTED MEDIA TYPE\r\n')

        x = requests.post(
            f'{api0}/thingsdb',
            json={'type': 'query', 'code': '42'},
            auth=('admin', 'XXX'),
        )
        self.assertEqual(x.status_code, 401)
        self.assertEqual(x.text, 'UNAUTHORIZED\r\n')

        x = requests.post(
            f'{api0}//stuff',
            json={'type': 'query', 'code': '42'},
            headers={'Authorization': 'TOKEN 1234567890abcdefghij'}
        )
        self.assertEqual(x.status_code, 401)
        self.assertEqual(x.text, 'UNAUTHORIZED\r\n')

        x = requests.post(
            f'{api0}//stuff',
            json={'type': 'query', 'code': '42'},
            headers={'Authorization': f'TOKEN {token}'}
        )
        self.assertEqual(x.status_code, 403)
        self.assertRegex(
            x.text,
            r'user `test` is missing the required privileges \(`QUERY`\) '
            r'on scope `@collection:stuff`.*')

        x = requests.post(
            f'{api0}/x/unknown',
            json={'type': 'query', 'code': '42'},
            auth=('admin', 'pass'),
        )
        self.assertEqual(x.status_code, 404)
        self.assertEqual(x.text, 'NOT FOUND\r\n')

        x = requests.post(
            f'{api0}//stuff',
            json={'type': 'query', 'code': '.a'},
            auth=('admin', 'pass'),
        )
        self.assertEqual(x.status_code, 404)
        self.assertRegex(
            x.text,
            r'thing `#[0-9]+` has no property `a` \(-54\)\s*')

        x = requests.post(
            f'{api0}//stuff',
            json={
                'type': 'query',
                'code': 'x',
                'vars': {'x': 18446744073709551616}
            },
            auth=('admin', 'pass'),
        )
        self.assertEqual(x.status_code, 400)
        self.assertEqual(x.text, 'BAD REQUEST\r\n')

        x = requests.post(
            f'{api0}//stuff',
            json={'type': 'query', 'code': '1 / 0;'},
            auth=('admin', 'pass'),
        )
        self.assertEqual(x.status_code, 422)
        self.assertEqual(x.text, 'division or modulo by zero (-58)\r\n')

        data = {
            'type': 'query',
            'code': 'unkown();'
        }

        x = requests.post(
            f'{api0}/n/1',
            json=data,
            auth=('admin', 'pass'),
        )

        self.assertEqual(x.status_code, 404)
        self.assertEqual(x.text, 'function `unkown` is undefined (-54)\r\n')

        x = requests.post(
            f'{api0}//stuff',
            json={'code': '42;'},
            auth=('admin', 'pass'),
        )

        self.assertEqual(x.status_code, 400)
        self.assertRegex(x.text, r'invalid API request.*')

    async def test_token_auth(self, api0, api1, token):
        data = {'type': 'query', 'code': '42'}
        x = requests.post(
            f'{api0}/t',
            json=data,
            headers={'Authorization': f'TOKEN {token}'}
        )
        self.assertEqual(x.status_code, 200)
        self.assertEqual(x.json(), 42)

        x = requests.post(
            f'{api0}/t',
            json=data,
            headers={'Authorization': f'Bearer {token}'}
        )
        self.assertEqual(x.status_code, 200)
        self.assertEqual(x.json(), 42)

        x = requests.post(
            f'{api0}/t',
            data=msgpack.dumps(data),
            auth=('admin', 'pass'),
            headers={'Content-Type': 'application/msgpack'}
        )

        self.assertEqual(x.status_code, 200)
        self.assertEqual(msgpack.unpackb(x.content, raw=False), 42)

    async def test_long_request(self, api0, api1, token):
        code = r''' "just another simple test string!!!"; ''' * 3000
        data = {'type': 'query', 'code': code}
        x = requests.post(
            f'{api0}/t',
            json=data,
            headers={'Authorization': f'TOKEN {token}'})

        self.assertEqual(x.status_code, 200)
        self.assertEqual(x.json(), "just another simple test string!!!")

    async def test_run(self, api0, api1, token):
        data = {'type': 'query', 'code': 'new_procedure("addone", |x| x+1);'}
        x = requests.post(
            f'{api0}//stuff',
            json=data,
            auth=('admin', 'pass'),
        )

        data = {'type': 'run', 'name': 'addone', 'args': [42]}

        x = requests.post(
            f'{api0}//stuff',
            json=data,
            auth=('admin', 'pass'),
        )
        self.assertEqual(x.status_code, 200)
        self.assertEqual(x.json(), 43)

        data = {'type': 'run', 'name': 'addone', 'args': {'x': 6}}

        x = requests.post(
            f'{api0}//stuff',
            json=data,
            auth=('admin', 'pass'),
        )
        self.assertEqual(x.status_code, 200)
        self.assertEqual(x.json(), 7)

        data = {'type': 'run', 'name': 'addone', 'args': {'x': 7, 'y': 5}}

        x = requests.post(
            f'{api0}//stuff',
            json=data,
            auth=('admin', 'pass'),
        )
        self.assertEqual(x.status_code, 200)
        self.assertEqual(x.json(), 8)


if __name__ == '__main__':
    run_test(TestHTTPAPI())

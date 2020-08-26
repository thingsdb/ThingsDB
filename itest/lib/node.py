import sys
import os
import asyncio
import subprocess
import platform
import configparser
import psutil
import logging
import io
import re
import threading
from .vars import THINGSDB_BIN
from .vars import THINGSDB_MEMCHECK
from .vars import THINGSDB_NODE_OUTPUT
from .vars import THINGSDB_TESTDIR
from .vars import THINGSDB_VERBOSE
from .color import Color
from thingsdb.exceptions import NodeError


MEM_PROC = \
    'memcheck-amd64-' if platform.architecture()[0] == '64bit' else \
    'memcheck-x86-li'


def get_file_content(fn):
    with open(fn, 'r') as f:
        data = f.read()
        return data


class Node:
    def __init__(self, n: int, test_name: str, **options):
        self.proc = None
        self.n = n

        test_name = test_name.replace(' ', '_')
        self.outfn = os.path.join(
            THINGSDB_TESTDIR,
            f'{test_name}-node{n}-out.log'
        )
        self.errfn = os.path.join(
            THINGSDB_TESTDIR,
            f'{test_name}-node{n}-err.log'
        )

        self.listen_client_port = 9200 + n
        self.http_api_port = 9210 + n
        self.listen_node_port = 9220 + n

        # can be used for clients to connect
        self.address_info = ('localhost', self.listen_client_port)

        self.bind_client_addr = options.pop('bind_client_addr', '::')
        self.bind_node_addr = options.pop('bind_node_addr', '::')

        self.ip_support = options.pop('ip_support', 'ALL')
        self.pipe_client_name = options.pop('pipe_client_name', None)
        self.threshold_full_storage = options.pop('threshold_full_storage', 10)

        self.storage_path = os.path.join(THINGSDB_TESTDIR, f'tdb{n}')
        self.cfgfile = os.path.join(THINGSDB_TESTDIR, f't{n}.conf')

        self.name = \
            f'Node{n}<{self.listen_client_port}/{self.listen_node_port}>'

        assert not options, f'invalid options: {",".join(options)}'

    async def init(self):
        self.start(init=True)
        await self.expect(
            'Well done, you successfully initialized ThingsDB!!', timeout=5)

    async def wait_join(self, secret: str):
        self.start(secret=secret)
        await self.expect(
            'start listening for node connections', timeout=5)

    async def run(self):
        self.start()
        await self.expect(
            'start listening for node connections', timeout=5)

    async def init_and_run(self):
        await self.init()

    async def join(self, client, secret='my_secret', attempts=1):
        await self.wait_join(secret)
        while True:
            try:
                await client.query(f'''
                    new_node("{secret}", "127.0.0.1", {self.listen_node_port});
                ''', scope='@thingsdb')
            except NodeError as e:
                attempts -= 1
                if not attempts:
                    raise e
            else:
                break
            await asyncio.sleep(1)

    async def join_until_ready(self, client, secret='my_secret', timeout=60):
        await self.wait_join(secret)
        node_id = await client.query(f'''
            new_node("{secret}", "127.0.0.1", {self.listen_node_port});
        ''', scope='@thingsdb')
        while timeout is None or timeout:
            try:
                res = await client.nodes_info()
            except NodeError:
                pass
            else:
                for node in res:
                    if node['node_id'] == node_id and \
                            node['status'] == 'READY':
                        return
            timeout -= 1
            await asyncio.sleep(1)

    @staticmethod
    def _handle_output(node, r):
        """Runs in another thread."""
        r = os.fdopen(r, 'r')
        for line in r:
            node.queue.put_nowait(line)
            if THINGSDB_NODE_OUTPUT is True or (
                    isinstance(THINGSDB_NODE_OUTPUT, int) and
                    node.n == THINGSDB_NODE_OUTPUT):
                print(Color.node(node.n, line), end='')

    async def _expect(self, pattern):
        while True:
            line = await self.queue.get()
            if pattern in line:
                return

    async def expect(self, pattern, timeout=10):
        await asyncio.wait_for(self._expect(pattern), timeout=timeout)

    def write_config(self):
        config = configparser.RawConfigParser()
        config.add_section('thingsdb')

        config.set('thingsdb', 'listen_client_port', self.listen_client_port)
        config.set('thingsdb', 'listen_node_port', self.listen_node_port)
        config.set('thingsdb', 'http_api_port', self.http_api_port)

        config.set('thingsdb', 'bind_client_addr', self.bind_client_addr)
        config.set('thingsdb', 'bind_node_addr', self.bind_node_addr)
        config.set(
            'thingsdb',
            'threshold_full_storage',
            self.threshold_full_storage)

        config.set('thingsdb', 'ip_support', self.ip_support)

        if self.pipe_client_name is not None:
            config.set('thingsdb', 'pipe_client_name',  self.pipe_client_name)

        config.set('thingsdb', 'storage_path', self.storage_path)

        with open(self.cfgfile, 'w') as configfile:
            config.write(configfile)

        try:
            os.mkdir(self.storage_path)
        except FileExistsError:
            pass

    def version(self):
        command = THINGSDB_MEMCHECK + [
            THINGSDB_BIN,
            '--version'
        ]

        p = subprocess.Popen(
            command,
            cwd=os.getcwd(),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

        out, err = p.communicate()
        assert p.returncode == 0

        r = re.compile(r'.*(version: [0-9]+\.[0-9]+\.[0-9]+).*')
        o = str(out)
        m = r.match(o)
        assert m

        if THINGSDB_NODE_OUTPUT is True or (
                isinstance(THINGSDB_NODE_OUTPUT, int) and
                self.n == THINGSDB_NODE_OUTPUT):
            print(Color.node(self.n, m.group(1)))

    def start(self, init=None, secret=None):
        self.queue = asyncio.Queue()

        command = THINGSDB_MEMCHECK + [
            THINGSDB_BIN,
            '--config', self.cfgfile,
            '--log-level', 'debug' if THINGSDB_VERBOSE else 'info',
            '--log-colorized'
        ]

        if init:
            command.append('--init')
        elif secret:
            command.extend(['--secret', secret])

        r, w = os.pipe()
        w = os.fdopen(w, 'w')

        t = threading.Thread(target=self._handle_output, args=(self, r))
        t.start()

        self.proc = subprocess.Popen(
            command,
            cwd=os.getcwd(),
            stderr=w,
            stdout=w,
        )
        logging.debug(f'{self.name} started using PID `{self.proc.pid}`')

    def is_active(self):
        return False if self.proc is None else psutil.pid_exists(self.proc.pid)

    def kill(self):
        command = f'kill -9 {self.proc.pid}'
        logging.info(f'execute: `{command}``')
        os.system(command)
        self.proc = None

    async def stop(self):
        if self.is_active():
            os.system('kill {}'.format(self.proc.pid))
            await asyncio.sleep(0.2)

            if self.is_active():
                os.system('kill {}'.format(self.proc.pid))
                await asyncio.sleep(1.0)

                if self.is_active():
                    return False

        self.proc = None
        return True

    async def shutdown(self, timeout=20):
        if self.is_active():
            os.system('kill {}'.format(self.proc.pid))

            while timeout:
                self.proc.communicate()
                if not self.is_active():
                    break
                await asyncio.sleep(1.0)
                timeout -= 1

            assert (self.proc.returncode == 0)

    def soft_kill(self):
        if self.is_active():
            os.system('kill {}'.format(self.proc.pid))
        self.proc = None

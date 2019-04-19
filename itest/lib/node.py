import os
import asyncio
import subprocess
import platform
import configparser
import psutil
from .vars import THINGSDB_TERMINAL
from .vars import THINGSDB_TERM_GEOMETRY
from .vars import THINGSDB_TERM_KEEP
from .vars import THINGSDB_MEMCHECK
from .vars import THINGSDB_BIN
from .vars import THINGSDB_TESTDIR
from .color import Color


MEM_PROC = \
    'memcheck-amd64-' if platform.architecture()[0] == '64bit' else \
    'memcheck-x86-li'


def get_file_content(fn):
    with open(fn, 'r') as f:
        data = f.read()
        return data


class Node:
    def __init__(self, n: int, test_name: str, **options):
        self.pid = None
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
        self.listen_node_port = 9220 + n

        self.bind_client_addr = options.pop('bind_client_addr', '::')
        self.bind_node_addr = options.pop('bind_node_addr', '::')

        self.ip_support = options.pop('ip_support', 'ALL')
        self.pipe_client_name = options.pop('pipe_client_name', None)

        self.storage_path = os.path.join(THINGSDB_TESTDIR, f'tdb{n}')
        self.cfgfile = os.path.join(THINGSDB_TESTDIR, f't{n}.conf')

        self.name = \
            f'Node{n}<{self.listen_client_port}/{self.listen_node_port}>'

        assert not options, f'invalid options: {",".join(options)}'

    def write_config(self):
        config = configparser.RawConfigParser()
        config.add_section('thingsdb')

        config.set('thingsdb', 'listen_client_port', self.listen_client_port)
        config.set('thingsdb', 'listen_node_port', self.listen_node_port)

        config.set('thingsdb', 'bind_client_addr', self.bind_client_addr)
        config.set('thingsdb', 'bind_node_addr', self.bind_node_addr)

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

    def _get_pid_set(self):
        try:
            ret = set(map(int, subprocess.check_output([
                'pgrep',
                MEM_PROC if THINGSDB_MEMCHECK else 'siridb-server']).split()))
        except subprocess.CalledProcessError:
            ret = set()
        return ret

    def _start(self, init=None, secret=None):
        command = (
            f'"{THINGSDB_MEMCHECK}{THINGSDB_BIN} '
            f'--config {self.cfgfile}" '
            f'{"--init " if init else ""}'
            f'{f"--secret {secret}" if secret else ""}'
        )

        if THINGSDB_TERMINAL == 'xterm':
            self.proc = subprocess.Popen(
               f'xterm {"-hold " if THINGSDB_TERM_KEEP else ""}'
               f'-title {self.name} '
               f'-geometry {THINGSDB_TERM_GEOMETRY} '
               f'-e {command}',
               shell=True
            )
            return

        if THINGSDB_TERMINAL is None:
            with open(self.errfn, 'a') as err:
                with open(self.outfn, 'a') as out:
                    self.proc = subprocess.Popen(
                        command,
                        stderr=err,
                        stdout=out,
                        shell=True
                    )
            return
        assert 0, f'invalid THINGSDB_TERMINAL: `{THINGSDB_TERMINAL}`'

    async def astart(self, init=None, secret=None):
        prev = self._get_pid_set()

        self._start(init, secret)

        await asyncio.sleep(5)

        my_pid = self._get_pid_set() - prev

        if len(my_pid) != 1:
            if THINGSDB_TERMINAL is None:
                if os.path.exists(self.outfn) and os.path.getsize(self.outfn):
                    reasoninfo = get_file_content(self.outfn)
                elif os.path.exists(self.errfn):
                    reasoninfo = get_file_content(self.errfn)
                else:
                    reasoninfo = 'unknown'
                assert 0, (
                    f'{Color.error("Failed to start ThingsDB")}\n'
                    f'{Color.info(reasoninfo)}\n')
            else:
                assert 0, (
                    'Failed to start ThingsDB. A possible reason could '
                    'be that another process is using the same port.')

        self.pid = my_pid.pop()

    def is_active(self):
        return False if self.pid is None else psutil.pid_exists(self.pid)

    def kill(self):
        print("!!!!!!!!!!!! KILLL !!!!!!!!!!")
        os.system('kill -9 {}'.format(self.pid))
        self.pid = None

    async def stopstop(self):
        if self.is_active():
            os.system('kill {}'.format(self.pid))
            await asyncio.sleep(0.2)

            if self.is_active():
                os.system('kill {}'.format(self.pid))
                await asyncio.sleep(1.0)

                if self.is_active():
                    return False

        self.pid = None
        return True

    async def stop(self, timeout=20):
        if self.is_active():
            os.system('kill {}'.format(self.pid))

            while (timeout and self.is_active()):
                await asyncio.sleep(1.0)
                timeout -= 1

        if hasattr(self, 'proc'):
            self.proc.communicate()
            assert (self.proc.returncode == 0)

        if timeout:
            self.pid = None
            return True

        return False

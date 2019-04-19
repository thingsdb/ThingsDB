import os
import asyncio
import subprocess
import platform
import configparser
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
    def __init__(self, n: int, **options):
        self.pid = None
        self.n = n

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
            errfn = os.path.join(
                THINGSDB_TESTDIR,
                f'{self.test_title}-{self.name}-err.log'
            )
            outfn = os.path.join(
                THINGSDB_TESTDIR,
                f'{self.test_title}-{self.name}-out.log'
            )
            with open(errfn, 'a') as err:
                with open(outfn, 'a') as out:
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
            if self.TERMINAL is None:
                if os.path.exists(outfn) and os.path.getsize(outfn):
                    reasoninfo = get_file_content(outfn)
                elif os.path.exists(errfn):
                    reasoninfo = get_file_content(errfn)
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

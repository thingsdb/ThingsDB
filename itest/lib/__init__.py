import asyncio
import logging
import random
import subprocess
from .testbase import TestBase
from .task import Task
from .cleanup import cleanup as clean
from .cleanup import killall
from .node import Node
from .color import Color
from .vars import THINGSDB_BIN
from .vars import THINGSDB_KEEP_ON_ERROR
from .vars import THINGSDB_LOGLEVEL
from .vars import THINGSDB_MEMCHECK
from .vars import THINGSDB_NODE_OUTPUT
from .vars import THINGSDB_TESTDIR
from .vars import THINGSDB_VERBOSE


def default_test_setup(num_nodes=1, seed=None, **kwargs):
    if seed is not None:
        random.seed(seed)

    def wrapper(func):
        async def wrapped(self):
            self.nodes = [
                Node(n, self.title, **kwargs)
                for n in range(num_nodes)]

            for n, node in enumerate(self.nodes):
                setattr(self, f'node{n}', node)
                node.write_config()

            _ = await func(self)

            for node in self.nodes:
                _ = await node.shutdown()

        return wrapped

    return wrapper


async def _run_test(test, hide_version: bool = False):
    logger = logging.getLogger()
    logger.setLevel(THINGSDB_LOGLEVEL)
    task = Task(test.title)

    if not hide_version:
        try:
            result = subprocess.run(
                [THINGSDB_BIN, '--version'],
                capture_output=True,
                text=True,
                check=True
            )
            out = result.stdout.strip()
            lines = out.splitlines()
            lines[0] = f'   {lines[0]}'
            out = '\n'.join(lines[:5])
            print(Color.info(out))
        except Exception:
            pass

    logging.info(f"""
Test Settings:
  THINGSDB_BIN: {THINGSDB_BIN}
  THINGSDB_KEEP_ON_ERROR: {THINGSDB_KEEP_ON_ERROR}
  THINGSDB_LOGLEVEL: {THINGSDB_LOGLEVEL}
  THINGSDB_MEMCHECK: {THINGSDB_MEMCHECK}
  THINGSDB_NODE_OUTPUT: {THINGSDB_NODE_OUTPUT}
  THINGSDB_TESTDIR: {THINGSDB_TESTDIR}
  THINGSDB_VERBOSE: {THINGSDB_VERBOSE}
""")

    try:
        await test.async_run()
    except Exception as e:
        task.stop(success=False)
        if not THINGSDB_KEEP_ON_ERROR:
            killall()
        raise e
    else:
        task.stop(success=True)

    logger.setLevel('CRITICAL')

    await task.task


def run_test(test: TestBase, hide_version: bool = False):
    loop = asyncio.new_event_loop()
    clean()
    loop.run_until_complete(_run_test(test, hide_version))


INT_MIN = -9223372036854775808
INT_MAX = 9223372036854775807

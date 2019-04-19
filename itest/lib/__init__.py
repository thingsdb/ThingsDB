import asyncio
import logging
from .testbase import TestBase
from .task import Task
from .cleanup import cleanup
from .node import Node
from .vars import THINGSDB_BIN
from .vars import THINGSDB_LOGLEVEL
from .vars import THINGSDB_MEMCHECK
from .vars import THINGSDB_TERMINAL
from .vars import THINGSDB_TERM_GEOMETRY
from .vars import THINGSDB_TERM_KEEP
from .vars import THINGSDB_TESTDIR


def default_test_setup(num_nodes=1, **kwargs):
    def wrapper(func):
        async def wrapped(self):
            self.nodes = [
                Node(n, self.title, **kwargs)
                for n in range(num_nodes)]

            for n, node in enumerate(self.nodes):
                setattr(self, f'node{n}', node)
                node.write_config()

            close = await func(self)

            if THINGSDB_TERMINAL is None or THINGSDB_TERM_KEEP is not True:
                for node in self.nodes:
                    result = await node.stop()
                    self.assertTrue(
                        result,
                        msg=f'Node {node.name} did not close correctly')

        return wrapped

    return wrapper


async def _run_test(test):
    logger = logging.getLogger()
    logger.setLevel(THINGSDB_LOGLEVEL)
    task = Task(test.title)

    logging.info(f"""
Test Settings:
  THINGSDB_BIN: {THINGSDB_BIN}
  THINGSDB_TESTDIR: {THINGSDB_TESTDIR}
  THINGSDB_MEMCHECK: {THINGSDB_MEMCHECK}
  THINGSDB_TERMINAL: {THINGSDB_TERMINAL}
  THINGSDB_TERM_KEEP: {THINGSDB_TERM_KEEP}
  THINGSDB_TERM_GEOMETRY: {THINGSDB_TERM_GEOMETRY}
  THINGSDB_LOGLEVEL: {THINGSDB_LOGLEVEL}
""")

    try:
        await test.run()
    except Exception as e:
        task.stop(success=False)
        raise e
    else:
        task.stop(success=True)

    logger.setLevel('CRITICAL')

    await task.task


def run_test(test: TestBase):
    loop = asyncio.get_event_loop()
    cleanup()
    loop.run_until_complete(_run_test(test))

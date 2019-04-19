import asyncio
import logging
from .testbase import TestBase
from .task import Task
from .cleanup import cleanup


def default_test_setup(num_nodes=1, **kwargs):
    def wrapper(func):
        async def wrapped(self):
            self.db = SiriDB(**kwargs)

            self.servers = [
                Server(n, title=self.title, **kwargs) for n in range(nservers)]
            for n, server in enumerate(self.servers):
                setattr(self, 'server{}'.format(n), server)
                setattr(self, 'client{}'.format(n), Client(self.db, server))
                server.create()
                await server.start()

            time.sleep(2.0)

            await self.db.create_on(self.server0, sleep=5)

            close = await func(self)

            if Server.TERMINAL is None or Server.HOLD_TERM is not True:
                for server in self.servers:
                    result = await server.stop()
                    self.assertTrue(
                        result,
                        msg='Server {} did not close correctly'.format(
                            server.name))

        return wrapped
    return wrapper


async def _run_test(test, loglevel):
    logger = logging.getLogger()
    logger.setLevel(loglevel)
    task = Task(test.title)

    try:
        await test.run()
    except Exception as e:
        task.stop(success=False)
        raise e
    else:
        task.stop(success=True)

    logger.setLevel('CRITICAL')

    await task.task


def run_test(test: TestBase, loglevel='CRITICAL'):
    loop = asyncio.get_event_loop()
    cleanup()
    loop.run_until_complete(_run_test(test, loglevel))

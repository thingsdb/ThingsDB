import unittest


class TestBase(unittest.TestCase):

    title = 'no test title'

    async def run(self):
        raise NotImplementedError('run must be implemented')

    async def run_tests(self, *args, **kwargs):
        for attr, f in self.__class__.__dict__.items():
            if attr.startswith('test_') and callable(f):
                await f(self, *args, **kwargs)

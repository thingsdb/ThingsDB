import unittest


class TestBase(unittest.TestCase):

    title = 'no test title'

    async def run(self):
        raise NotImplementedError('run must be implemented')

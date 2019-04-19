import unittest


class TestBase(unittest.TestCase):

    async def run(self):
        raise NotImplementedError('run must be implemented')

from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase


class TestMultiNode(TestBase):

    @default_test_setup(2)
    async def run(self):
        pass


if __name__ == '__main__':
    run_test(TestMultiNode())

from lib.testbase import TestBase
from lib import run_test
from lib import default_test_setup


class TestMultiNode(TestBase):

    @default_test_setup(2)
    def run(self):
        pass


if __name__ == '__main__':
    run_test(TestMultiNode())

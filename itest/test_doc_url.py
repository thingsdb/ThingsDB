#!/usr/bin/env python
import asyncio
import pickle
import logging
import time
import sys
import os
import re
import urllib.request
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client

THINGSDB_PATH = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
INC_PATH = os.path.join(THINGSDB_PATH, 'inc')
DOC_FN = os.path.join(INC_PATH, 'doc.h')
RE_DOC_DOCS = re.compile(r'#define DOC_DOCS.*"(http[a-zA-Z0-9\:\.\/\-_]*)"')
RE_DOC = re.compile(r'#define DOC_.*DOC_SEE\("([a-zA-Z0-9\.\/\-_]+)"\)')


class TestDocUrl(TestBase):

    title = 'Test documentation url'

    async def run(self):
        with open(DOC_FN, 'r') as f:
            lines = f.readlines()

        lines = iter(lines)
        url = self.get_url(lines)
        self.test_url(lines, url)

    @staticmethod
    def get_url(lines):
        for line in lines:
            m = RE_DOC_DOCS.match(line)
            if m:
                return m.group(1)
        raise ValueError('DOC_DOCS not defined in doc.h')

    @staticmethod
    def test_url(lines, url):
        for line in lines:
            m = RE_DOC.match(line)
            if m:
                testurl = f'{url}{m.group(1)}'
                logging.info(f'testing `{testurl}`...')
                try:
                    with urllib.request.urlopen(testurl) as response:
                        html = response.read()
                except urllib.error.HTTPError as e:
                    raise ValueError(f'`{e}` happens with `{testurl}`')


if __name__ == '__main__':
    run_test(TestDocUrl())

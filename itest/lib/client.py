import sys
import asyncio
import functools
import logging
import random
import time
from thingsdb.client import Client


async def get_client(node: Node, username='admin', password='pass'):
    client = Client()
    await client.connect(*node.address_info)
    await client.authenticate(username, password)
    return client

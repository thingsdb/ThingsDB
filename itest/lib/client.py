import sys
import asyncio
import functools
import logging
import random
import time
from thingsdb.client import Client
from .node import Node


async def get_client(*nodes, username='admin', password='pass', **kwargs):
    client = Client(**kwargs)
    await client.connect_pool([node.address_info for node in nodes])
    await client.authenticate(username, password)
    return client

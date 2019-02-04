import asyncio
from thingsdb.client import Client


async def example():
    await client.connect('server.local', 9200)
    await client.authenticate('admin', 'pass')
    res = await client.query(r'''
        'Hello World!!'.lower();
    ''', target='stuff')
    print(res)


client = Client()
asyncio.get_event_loop().run_until_complete(example())

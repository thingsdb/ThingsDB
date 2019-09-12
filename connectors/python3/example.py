import asyncio
from thingsdb.client import Client


async def example():
    await client.connect('localhost')
    await client.authenticate('admin', 'pass')

    res = await client.query(r'''
        @:stuff
        .greet = 'Hello World!!';
    ''')

    print(res)


client = Client()
asyncio.get_event_loop().run_until_complete(example())

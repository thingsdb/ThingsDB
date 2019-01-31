import asyncio
from thingsdb.client import Client

client = Client()
loop = asyncio.get_event_loop()


async def example():
    # replace `localhost` with your ThingsDB server address
    await client.connect('localhost', 9200)

    # replace `admin` with yout username and `pass` with your password
    await client.authenticate('admin', 'pass')

# run the example
loop.run_until_complete(example())

# the will close the client in a nice way
client.close()
loop.run_until_complete(client.wait_closed())

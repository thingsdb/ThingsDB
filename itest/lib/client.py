from thingsdb.client import Client


async def get_client(*nodes, username='admin', password='pass', **kwargs):
    client = Client(**kwargs)
    await client.connect_pool([node.address_info for node in nodes])
    await client.authenticate(username, password)
    return client

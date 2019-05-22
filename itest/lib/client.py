from thingsdb.client import Client


async def get_client(*nodes, username='admin', password='pass', **kwargs):
    client = Client(**kwargs)
    await client.connect_pool(
        pool=[node.address_info for node in nodes],
        username=username,
        password=password)
    return client

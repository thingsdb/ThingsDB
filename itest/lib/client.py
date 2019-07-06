from thingsdb.client import Client


async def get_client(*nodes, auth=['admin', 'pass'], **kwargs):
    client = Client(**kwargs)
    await client.connect_pool(
        pool=[node.address_info for node in nodes],
        auth=auth)
    return client

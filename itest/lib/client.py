from thingsdb.client import Client


async def get_client(*nodes, auth=['admin', 'pass'], **kwargs):
    client = Client(**kwargs)
    if isinstance(auth, str):
        auth = [auth]
    await client.connect_pool(
        [node.address_info for node in nodes],
        *auth)
    return client

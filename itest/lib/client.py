from thingsdb.client import Client


async def get_client(
        *nodes,
        auth=['admin', 'pass'],
        auto_reconnect=True,
         **kwargs):
    client = Client(
        auto_reconnect=auto_reconnect,
        ssl=nodes[0].use_ssl,
        **kwargs)
    if isinstance(auth, str):
        auth = [auth]
    if auto_reconnect:
        await client.connect_pool(
            [node.address_info for node in nodes],
            *auth)
    else:
        await client.connect(*nodes[0].address_info)
        await client.authenticate(*auth)
    return client

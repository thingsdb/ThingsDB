from thingsdb.client import Client
from thingsdb.scope import Scope


async def create_target(client: Client, name: str):
    target = Scope(name)
    await client.new_collection(target)
    return target

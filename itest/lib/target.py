from thingsdb.client import Client
from thingsdb.target import Target


async def create_target(client: Client, name: str):
    target = Target(name)
    await client.new_collection(target)
    return target

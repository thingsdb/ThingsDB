from .collection import Collection
from .target import Target

ROOT_TARGET = 0


class Root:

    async def new_collection(self, target: Target):
        await self.query(
            f'new_collection("{target._target}")',
            target=self.thingsdb)

    async def get_collection(self, name):
        collection_id = await self.query(f'id()', target=name)
        collection = Collection(self, collection_id)
        await collection.fetch()
        return collection

from .collection import Collection


ROOT_TARGET = 0


class Root:

    async def new_collection(self, name):
        await self.query(f'new_collection("{name}")', target=ROOT_TARGET)
        return

    async def get_collection(self, name):
        collection_id = await self.query(f'id()', target=name)
        collection = Collection(self, collection_id)
        await collection.fetch()
        return collection

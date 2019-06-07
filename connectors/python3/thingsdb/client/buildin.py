from .scope import Scope


class Buildin:

    async def new_collection(self, name):
        return await self.query(
            f'new_collection("{name}")',
            target=self.thingsdb)

    async def collection(self, name_or_id):
        return await self.query(
            f'collection("{name_or_id}")',
            target=self.thingsdb)

    async def node(self):
        return await self.query(
            f'node()',
            target=self.node)

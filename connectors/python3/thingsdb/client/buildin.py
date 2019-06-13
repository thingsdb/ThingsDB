from . import scope


class Buildin:

    async def new_collection(self, name):
        return await self.query(
            f'new_collection("{name}")',
            target=scope.thingsdb)

    async def collection(self, name_or_id):
        return await self.query(
            f'collection("{name_or_id}")',
            target=scope.thingsdb)

    async def node(self):
        return await self.query(
            f'node()',
            target=scope.node)

    async def nodes(self):
        return await self.query(
            f'nodes()',
            target=scope.node)

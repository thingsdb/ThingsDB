from .scope import node, thingsdb, Scope
from typing import Union as U


class Buildin:

    async def collection(self, collection: U[Scope, int, str]):
        if isinstance(collection, Scope):
            collection = collection._scope
        return await self.query(f'collection({collection!r})', target=thingsdb)

    async def collections(self):
        return await self.query('collections()', target=thingsdb)

    async def counters(self):
        return await self.query('counters()', target=node)

    async def del_collection(self, collection: U[Scope, int, str]):
        if isinstance(collection, Scope):
            collection = collection._scope
        return await self.query(
            f'del_collection({collection!r})',
            target=thingsdb)

    async def del_user(self, name: str):
        return await self.query(f'del_user({name!r})', target=thingsdb)

    async def grant(self, target: U[Scope, int, str], user: str, mask: int):
        if isinstance(target, Scope):
            target = target._scope
        return await self.query(
            f'grant({target!r}, {user!r}, {mask})',
            target=thingsdb)

    async def new_collection(self, name: str):
        return await self.query(f'new_collection({name!r})', target=thingsdb)

    async def node(self):
        return await self.query('node()', target=node)

    async def nodes(self):
        return await self.query('nodes()', target=node)

    async def reset_counters(self):
        return await self.query('reset_counters()', target=node)

    async def revoke(self, target: U[Scope, int, str], user: str, mask: int):
        if isinstance(target, Scope):
            target = target._scope
        return await self.query(
            f'grant({target!r}, {user!r}, {mask})',
            target=thingsdb)

    async def shutdown(self):
        return await self.query('shutdown()', target=node)

    async def users(self):
        return await self.query('users()', target=thingsdb)
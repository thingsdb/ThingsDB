import datetime
from .scope import node, thingsdb, Scope
from typing import Union as U


class Buildin:

    async def collection_info(self, collection: U[Scope, int, str]):
        if isinstance(collection, Scope):
            collection = collection._scope
        return await self.query(
            f'collection_info({collection!r})',
            target=thingsdb)

    async def collections_info(self):
        return await self.query('collections_info()', target=thingsdb)

    async def counters(self):
        return await self.query('counters()', target=node)

    async def del_collection(self, collection: U[Scope, int, str]):
        if isinstance(collection, Scope):
            collection = collection._scope
        return await self.query(
            f'del_collection({collection!r})',
            target=thingsdb)

    async def del_expired(self):
        return await self.query('del_expired()', target=thingsdb)

    async def del_token(self, key: str):
        return await self.query(f'del_token({key!r})', target=thingsdb)

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

    async def new_token(
            self,
            user: str,
            expiration_time: datetime.datetime = None,
            description: str = ''):

        if expiration_time is None:
            expiration_time = 'nil'
        else:
            expiration_time = int(datetime.datetime.timestamp(expiration_time))

        return await self.query(
            f'new_token({user!r}, {expiration_time}, {description!r})',
            target=thingsdb)

    async def node_info(self):
        return await self.query('node_info()', target=node)

    async def nodes_info(self):
        return await self.query('nodes_info()', target=node)

    async def reset_counters(self):
        return await self.query('reset_counters()', target=node)

    async def revoke(self, target: U[Scope, int, str], user: str, mask: int):
        if isinstance(target, Scope):
            target = target._scope
        return await self.query(
            f'grant({target!r}, {user!r}, {mask})',
            target=thingsdb)

    async def set_log_level(self, log_level: str):
        assert log_level in ('DEBUG', 'INFO', 'WARNING', 'ERROR', 'CRITICAL')
        return await self.query(f'set_log_level({log_level})', target=node)

    async def set_zone(self, zone: int):
        return await self.query(f'set_zone({zone})', target=node)

    async def shutdown(self):
        return await self.query('shutdown()', target=node)

    async def user_info(self, user: str = None):
        if user is None:
            return await self.query('user_info()', target=thingsdb)
        return await self.query(f'user_info({user!r})', target=thingsdb)

    async def users_info(self):
        return await self.query('users_info()', target=thingsdb)

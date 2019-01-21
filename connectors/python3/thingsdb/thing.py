import asyncio
from .onwatch import OnWatch
from .wrap import wrap
from .arr import Arr


class Thing:

    __slots__ = (
        'data',  # public binding
        '_client',
        '_collection',
        '_id',
        '_props',
        '_attrs',
        '_on_watch',
        '_is_fetched',
    )

    def _init(self, collection, id):
        self._collection = collection
        self._id = id
        self._props = {}
        self._attrs = None
        self._on_watch = OnWatch
        self._is_fetched = False

    def __new__(cls, collection, id):
        client = collection._client
        thing = client._things.get(id)
        if thing is None:
            thing = client._things[id] = super().__new__(cls)
            thing._init(collection, id)
        return thing

    def __repr__(self):
        return f'{{#:{self._id}}}'

    async def _query(self, query, **kwargs):
        return await self._collection._client.query(
            query,
            target=self._collection._id,
            **kwargs)

    def _unpack(self, prop, value, _arrtype=(list, tuple)):
        if isinstance(value, dict):
            if '#' in value:
                thing_id = value.pop('#')
                thing = Thing(self._collection, thing_id)
                for p, v in value.items():
                    thing._assign(p, v)
                return thing

        elif isinstance(value, _arrtype):
            return Arr(self, prop, (self._unpack(None, v) for v in value))
        return value

    def _assign(self, prop, value):
        self._props[prop] = self._unpack(prop, value)

    def __getattr__(self, name):
        if name in self._props:
            return self._props[name]
        is_fetched = 'fetched' if self._is_fetched else 'not fetched'
        raise AttributeError(
            f'{self} has no property `{name}` (thing is {is_fetched})')

    def _job_assign(self, job):
        for prop, value in job.items():
            self._assign(prop, value)

    def _job_del(self, job):
        del self._props[job]

    def _job_push(self, job):
        for prop, value in job.items():
            arr = self._props[prop]
            arr.extend((self._unpack(None, v) for v in value))
            arr.apply_watch(slice(-len(value), None))

    def _job_rename(self, job):
        for old_prop, new_prop in job.items():
            self._props[new_prop] = self._props.pop(old_prop)

    def _job_set(self, job):
        for attr, value in job.items():
            self._attrs[attr].extend((self._unpack(None, v) for v in value))

    def _job_splice(self, job):
        for prop, value in job.items():
            index, count, _new, *items = value
            sl = slice(index, index+count)
            arr = self._props[prop]
            arr[sl] = (self._unpack(None, v) for v in value)
            arr.apply_watch(sl)

    def _job_unset(self, job):
        try:
            del self._attrs[job]
        except KeyError:
            pass

    def _apply_watch_class(self, watchclass, arr):
        client = self._collection._client
        asyncio.ensure_future(
            client.watch((t.set_watcher(watchclass) for t in arr)),
            loop=client._loop
        )

    async def assign(self, name, value, **kwargs):
        blobs = kwargs.pop('blobs', [])
        value = wrap(value, blobs)
        if blobs:
            kwargs['blobs'] = blobs

        await self._query(f'thing({self._id}).{name}={value!r}', **kwargs)

    async def fetch(self):
        thing_dict = await self._query(f'thing({self._id})')
        for prop, value in thing_dict.items():
            self._assign(prop, value)
        self._is_fetched = True

    async def watch(self, **kwargs):
        return await self._collection._client.watch(
            [self],
            collection=self._collection._id
        )

    async def unwatch(self, **kwargs):
        return await self._collection._client.unwatch(
            [self],
            collection=self._collection._id
        )

    def id(self):
        return self._id

    def is_fetched(self):
        return self._is_fetched

    def set_watcher(self, watchclass):
        assert watchclass.on_init
        assert watchclass.on_update
        assert watchclass.on_delete
        self._on_watch = watchclass
        return self

    def on_watch_init(self, thing_dict):
        for prop, value in thing_dict.items():
            self._assign(prop, value)
        self._is_fetched = True

    def on_watch_update(self, jobs, _map={
        'assign': _job_assign,
        'del': _job_del,
        'push': _job_push,
        'rename': _job_rename,
        'set': _job_set,
        'splice': _job_splice,
        'unset': _job_unset,
    }):
        for job_dict in jobs:
            for name, job in job_dict.items():
                jobfun = _map.get(name)
                if jobfun is None:
                    raise TypeError(f'unknown job `{name}` for thing {self}')
                jobfun(self, job)

    def on_watch_delete(self):
        self._collection._client._things.pop(self._id)

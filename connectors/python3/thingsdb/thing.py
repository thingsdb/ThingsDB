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
        '__weakref__',
    )

    def _init(self, collection, id):
        self._collection = collection
        self._id = id
        self._props = {}
        self._attrs = None
        self._on_watch = OnWatch

    def __new__(cls, collection, id):
        client = collection._client
        thing = client._things.get(id)
        if thing is None:
            thing = client._things[id] = super().__new__(cls)
            thing._init(collection, id)
        return thing

    def __repr__(self):
        return f't({self._id})'

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

    def _job_assign(self, job):
        for prop, value in job.items():
            self._assign(prop, value)

    def _job_del(self, job):
        del self._props[job]

    def _job_rename(self, job):
        for old_prop, new_prop in job.items():
            self._props[new_prop] = self._props.pop(old_prop)

    def _job_splice(self, job):
        for prop, value in job.items():
            index, count, new, *items = value
            arr = self._props[prop]
            arr[index:index+count] = (self._unpack(None, v) for v in items)
            arr.apply_watch(slice(index, index+new))

    async def assign(self, name, value, **kwargs):
        blobs = kwargs.pop('blobs', [])
        value = wrap(value, blobs)
        if blobs:
            kwargs['blobs'] = blobs

        await self._query(f't({self._id}).{name}={value!r}', **kwargs)

    def id(self):
        return self._id

    def collection(self):
        return self._collection

    def prop(self, prop, d=None):
        return self._props.get(prop, d)

    def set_watcher(self, watchclass):
        assert watchclass.on_init
        assert watchclass.on_update
        assert watchclass.on_delete
        self._on_watch = watchclass
        return self

    def on_watch_init(self, thing_dict):
        for prop, value in thing_dict.items():
            self._assign(prop, value)

    def on_watch_update(self, jobs, _map={
        'assign': _job_assign,
        'del': _job_del,
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

import time
import asyncio
import logging
import pprint
import signal
from thingsdb.client import Client
from thingsdb.client.scope import Scope
from thingsdb.exceptions import ThingsDBError
from thingsdb.exceptions import IndexError

'''

from thingsdb.client import Client
from thingsdb.exceptions import ThingsDBError
client = Client()
await client.connect('localhost', 9200)
await client.authenticate('iris', 'siri')
collection = await client.get_collection('Test')

await collection.watch()
await collection.assign('y', 123)

'''

interrupted = False


class ArrayOf(list):
    pass


def array_of(type_):
    return type('ArrayOf', (ArrayOf, ), {'type_': type_})


class Thing(dict):

    __strict__ = True

    __slots__ = (
        '_id',
        '_collection',
        'ready',
        '__weakref__',
    )

    def __init__(self, id, collection):
        self._id = id
        self._collection = collection
        self.ready = False
        collection._client._things[id] = self
        collection._wqueue.add(id)

    def __new__(cls, id, collection):
        if id is None:
            return super().__new__(cls)
        client = collection._client
        thing = client._things.get(id)
        if thing is None:
            thing = super().__new__(cls, id, collection)
        return thing

    def _check(self, attr, value):
        if not issubclass(attr, value.__class__):
            return value, False

        if issubclass(attr, Thing):
            thing_id = value.get('#')
            if thing_id is None:
                return value, False
            thing = attr(thing_id, self._collection)
            return thing, True

        if issubclass(attr, ArrayOf):
            arr = attr()
            is_valid = True
            for v in value:
                v, check = self._check(attr.type_, v)
                arr.append(v)
                if not check:
                    is_valid = False
            return arr, is_valid

        return value, True

    def _assign(self, prop, val):
        attr = getattr(self.__class__, prop, None)
        val, is_valid = (val, False) \
            if attr is None else self._check(attr, val)

        if not is_valid and self.__class__.__strict__:
            if attr is None:
                logging.debug(
                    f'property `{prop}` is not defined and will be ignored')
            else:
                logging.debug(
                    f'property `{prop}` is expecting '
                    f'type `{attr.__name__}`')
            return

        setattr(self, prop, val)

    def _job_assign(self, attrs):
        for prop, value in attrs.items():
            self._assign(prop, value)

    def _job_del(self, prop):
        try:
            delattr(self, prop)
        except AttributeError:
            logging.debug(
                f'cannot delete property `{prop}` because it is missing')

    def _job_rename(self, props):
        for old_prop, new_prop in props.items():
            try:
                value = getattr(self, old_prop)
            except AttributeError:
                logging.debug(
                    f'cannot rename property `{prop}` because it is missing')
            else:
                setattr(self, new_prop, value)

    def _job_splice(self, job):
        for prop, value in job.items():
            index, count, new, *items = value
            try:
                arr = getattr(self, prop)
            except AttributeError:
                logging.debug(
                    f'cannot use splice on property `{prop}` because '
                    f'the property is missing')
            else:
                print(arr.__class__)
                items, is_valid = self._check(arr.__class__, items)
                if not is_valid and self.__class__.__strict__:
                    logging.error(
                        f'splice on property `{prop}` has failed '
                        f'the checks')
                arr[index:index+count] = items

    async def _on_watch_init(self, thing_dict):
        for prop, value in thing_dict.items():
            self._assign(prop, value)
        self.ready = True
        self._collection._process_wqueue()

    async def _on_watch_update(self, jobs, _map={
        'assign': _job_assign,
        'del': _job_del,
        'rename': _job_rename,
        'splice': _job_splice,
    }):
        for job_dict in jobs:
            for name, job in job_dict.items():
                jobfun = _map.get(name)
                if jobfun is None:
                    raise TypeError(f'unknown job `{name}` for thing {self}')
                jobfun(self, job)
        self._collection._process_wqueue()


class Collection(Scope, Thing):

    __slots__ = (
        '_client',
        '_scope',
        '_wqueue',
    )

    _instance = None

    def __init__(self, client):
        Scope.__init__(self, self.__class__.__name__)
        self._client = client
        self._wqueue = set()
        asyncio.ensure_future(self._async_init(), loop=client._loop)

    def __new__(cls, client):
        if cls._instance is None:
            cls._instance = super().__new__(cls, None, None)
        return cls._instance

    async def _async_init(self):
        try:
            collection_id = await self._client.query('id()', target=self)
        except IndexError:
            collection_id = await self._client.query(
                f'new_collection("{self.__class__.__name__}");',
                target=self._client.thingsdb)
        Thing.__init__(self, collection_id, self)
        self._collection._process_wqueue()

    def _process_wqueue(self):
        if not self._wqueue:
            return
        asyncio.ensure_future(self._client.watch(
            list(self._wqueue),
            target=self), loop=self._client._loop)
        self._wqueue.clear()


class Label(Thing):

    name = str
    description = str


class Condition(Thing):

    name = str
    description = str
    labels = array_of(Label)


class OsData(Collection):

    labels = array_of(Label)
    conditions = array_of(Condition)


async def test():
    client = Client()

    await client.connect('localhost', 9200)
    await client.authenticate('admin', 'pass')

    osdata = OsData(client)

    while True:
        if interrupted:
            break
        print(osdata.labels)
        await asyncio.sleep(0.2)

    print('-----------------------------------------------------------------')


def signal_handler(signal, frame):
    print('Quit...')
    global interrupted
    interrupted = True


if __name__ == '__main__':
    signal.signal(signal.SIGINT, signal_handler)

    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)
    loop = asyncio.get_event_loop()
    loop.run_until_complete(test())

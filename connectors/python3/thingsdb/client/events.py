from .protocol import ON_NODE_STATUS
from .protocol import ON_WARN
from .protocol import ON_WATCH_INI
from .protocol import ON_WATCH_UPD
from .protocol import ON_WATCH_DEL
import abc


class Events(metaclass=abc.ABCMeta):

    def __init__(self, client):
        self.client = client
        self._evmap = {
            ON_NODE_STATUS: self.on_node_status,
            ON_WARN: self.on_warning,
            ON_WATCH_INI: self.on_watch_init,
            ON_WATCH_UPD: self.on_watch_update,
            ON_WATCH_DEL: self.on_watch_delete,
        }

    @abc.abstractmethod
    async def on_reconnect(self):
        """On re-connect
        Called after a re-concect is finished (inclusing authentication)
        """
        pass

    @abc.abstractmethod
    def on_node_status(self, status):
        """On node status
        status: String containing a `new` node status.
                Optional values:
                    - OFFLINE
                    - CONNECTING
                    - BUILDING
                    - SYNCHRONIZING
                    - AWAY
                    - AWAY_SOON
                    - SHUTTING_DOWN
                    - READY
        """
        pass

    @abc.abstractmethod
    def on_warning(self, warn):
        """On warning
        warn: a dictionary with `warn_msg` and `warn_code`. for example:

        {
            "warn_msg": "some warning message"
            "warn_code": 1
        }
        """
        pass

    @abc.abstractmethod
    def on_watch_init(self, data):
        pass

    @abc.abstractmethod
    def on_watch_update(self, data):
        pass

    @abc.abstractmethod
    def on_watch_delete(self, data):
        pass


class ModelEv(Events):

    def __init__(self, client):
        super().__init__client()
        self._things = weakref.WeakValueDictionary()  # watching these things

    async def on_reconnect(self):
        # re-watch all watching things
        collections = set()
        for t in self._things.values():
            t._to_wqueue()
            collections.add(t._collection)

        if collections:
            for collection in collections:
                await collection.go_wqueue()

    def on_node_status(self, status):
        pass

    def on_watch_init(self, data):
        thing_dict = data['thing']
        thing = self._things.get(thing_dict.pop('#'))
        if thing is None:
            return
        asyncio.ensure_future(
            thing.on_init(data['event'], thing_dict),
            loop=self._loop
        )

    def on_watch_update(self, data):
        thing = self._things.get(data.pop('#'))
        if thing is None:
            return

        asyncio.ensure_future(
            thing.on_update(data['event'], data.pop('jobs')),
            loop=self._loop
        )

    def on_watch_delete(self, data):
        thing = self._things.get(data.pop('#'))
        if thing is None:
            return

        asyncio.ensure_future(thing.on_delete(), loop=self._loop)

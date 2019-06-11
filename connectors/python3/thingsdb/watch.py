import asyncio
import logging
from .protocol import ON_WATCH_INI
from .protocol import ON_WATCH_UPD
from .protocol import ON_WATCH_DEL
from .protocol import ON_NODE_STATUS


class WatchMixin:

    def _on_watch_received(self, pkg):
        if pkg.tp == ON_WATCH_INI:
            return self._on_watch_init(pkg.data)
        if pkg.tp == ON_WATCH_UPD:
            return self._on_watch_update(pkg.data)
        if pkg.tp == ON_WATCH_DEL:
            return self._on_watch_delete(pkg.data)

    def _on_watch_init(self, data):
        thing_dict = data['thing']
        thing = self._things.get(thing_dict.pop('#'))
        if thing is None:
            return
        asyncio.ensure_future(
            thing._on_watch.on_init(thing, thing_dict),
            loop=self._loop
        )

    def _on_watch_update(self, data):
        thing = self._things.get(data.pop('#'))
        if thing is None:
            return

        asyncio.ensure_future(
            thing._on_watch.on_update(thing, data.pop('jobs')),
            loop=self._loop
        )

    def _on_watch_delete(self, data):
        thing = self._things.get(data.pop('#'))
        if thing is None:
            return

        asyncio.ensure_future(
            thing._on_watch.on_delete(thing, ),
            loop=self._loop
        )

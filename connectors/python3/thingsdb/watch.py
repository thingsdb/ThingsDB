from .protocol import ON_WATCH_INI
from .protocol import ON_WATCH_UPD
from .protocol import ON_WATCH_DEL


class WatchMixin:

    def _on_watch_received(self, pkg):
        if pkg.tp == ON_WATCH_INI:
            return self._on_watch_init(pkg.data)

    def _on_watch_init(self, data):
        thing_dict = data['thing']
        thing_id = thing_dict.pop('#')
        thing = self._things.get(thing_id)
        if thing is None:
            return

        for k, v in thing_dict:
            if isinstance(v, dict):
                # do something special
                pass
            else:
                thing._props[k] = v



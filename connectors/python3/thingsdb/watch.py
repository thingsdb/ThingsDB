from .protocol import ON_WATCH_INI
from .protocol import ON_WATCH_UPD
from .protocol import ON_WATCH_DEL


class WatchMixin:

    def _on_watch_received(self, pkg):
        if pkg.tp == ON_WATCH_INI:
            return self._on_watch_init(pkg.data)
        if pkg.tp == ON_WATCH_UPD:
            return self._on_watch_update(pkg.data)

    def _on_watch_init(self, data):
        thing_dict = data['thing']
        thing_id = thing_dict.pop('#')
        thing = self._things.get(thing_id)
        if thing is None:
            return

        for prop, value in thing_dict.items():
            thing._assign(prop, value)

    def _on_watch_update(self, data):
        thing_id = data.pop('#')
        thing = self._things.get(thing_id)
        if thing is None:
            return

        for job_dict in data['jobs']:
            for name, job in job_dict.items():
                if name == 'assign':
                    self._job_assign(thing, job)

    def _job_assign(self, thing, job):
        for prop, value in job.items():
            thing._assign(prop, value)

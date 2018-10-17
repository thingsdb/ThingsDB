from .thing import Elem
from .thing import thing_set_props
from .thing import thing_map_props
from .event import Event
from .protocol import TASK_PROPS_SET


class Db(Elem):
    def __init__(self, client, target, dmap):
        self._client = client
        self._target = target
        self._things = {}
        self._next_id = 0
        id = dmap.pop('_i')
        super().__init__(self, id)
        thing_set_props(self, dmap)

    def new_thing(self, **props):
        return Elem(self, self.new_id)

    @property
    def new_id(self):
        self._next_id += 1
        return -self._next_id

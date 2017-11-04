from .elem import Elem
from .elem import elem_set_props


class Db(Elem):
    def __init__(self, client, target, dmap):
        self._client = client
        self._target = target
        self._elems = {}
        id = dmap.pop('_i')
        super().__init__(self, id)
        elem_set_props(self, dmap)

from typing import Union
from .protocol import REQ_QUERY_COLLECTION
from .protocol import REQ_QUERY_NODE
from .protocol import REQ_QUERY_THINGSDB


class Scope:

    def __init__(self, scope: Union[int, str], _proto=REQ_QUERY_COLLECTION):
        if not isinstance(scope, (int, str)):
            raise TypeError(
                f'invalid scope type `{scope.__class__.__name__}`, '
                f'expecting `int` or `str`')
        self._scope = scope
        self._proto = _proto

    def is_collection(self) -> bool:
        return self._proto == REQ_QUERY_COLLECTION

    def name(self):
        if isinstance(self._scope, int):
            return f'collection:{self._scope}'
        return self._scope


thingsdb = Scope('scope:thingsdb', _proto=REQ_QUERY_THINGSDB)
node = Scope('scope:node', _proto=REQ_QUERY_NODE)

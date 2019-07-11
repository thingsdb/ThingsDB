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


def scope_is_collection(scope) -> bool:
    return scope._proto == REQ_QUERY_COLLECTION


def scope_get_name(scope):
    if isinstance(scope._scope, int):
        return f'collection:{scope._scope}'
    return scope._scope


# The scope names are checked by the `grant` and `revoke` functions and must
# end with `.thingsdb` and `.node` respectively
thingsdb = Scope('scope.thingsdb', _proto=REQ_QUERY_THINGSDB)
node = Scope('scope.node', _proto=REQ_QUERY_NODE)

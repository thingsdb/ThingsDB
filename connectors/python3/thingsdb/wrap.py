from .nil import Nil
from .boolean import Boolean
from .thing import Thing


class Repr:
    def __init__(self, repr_):
        self._repr = repr_

    def __repr__(self):
        return self._repr


nil = Repr('nil')
true = Repr('true')
false = Repr('false')


def wrap(value, blobs, nowrap=nowrap):

    if value is None:
        return nil
    if isinstance(value, bool):
        return true if value else false
    if isinstance(value, bytes):
        idx = len(blobs)
        blobs.append(blob)
        return Repr(f'blob({self._idx}')
    if isinstance(value, dict):
        pass
    if isinstance(value, (list, tuple)):
        return f"[{','.join(repr(wrap(v, blobs)) for v in value)}]"
    if isinstance(value, nowrap):
        return value

    raise TypeError(f'cannot wrap type {type(value)}')



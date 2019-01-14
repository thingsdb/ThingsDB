from .nil import Nil
from .boolean import Boolean
from .thing import Thing
from .blob import Blob


def wrap(value, blob, nowrap=(int, float, str, Thing, Nil, Boolean, Blob)):
    if value is None:
        return Nil()
    if isinstance(value, bool):
        return Boolean(value)
    if isinstance(value, bytes):
        return Blob(value, blobs)
    if isinstance(value, nowrap):
        return value

    raise TypeError(f'cannot wrap type {type(value)}')
class Wrap:
    def __init__(self, repr_):
        self._repr = repr_

    def __repr__(self):
        return self._repr

    @staticmethod
    def nowrap():
        from .thing import Thing
        return (int, float, str, Thing, Wrap)


nil = Wrap('nil')
true = Wrap('true')
false = Wrap('false')


def wrap(value, blobs):

    if value is None:
        return nil
    if isinstance(value, bool):
        return true if value else false
    if isinstance(value, bytes):
        idx = len(blobs)
        blobs.append(blob)
        return Wrap(f'blob({self._idx}')
    if isinstance(value, dict):
        return Wrap(f"{{{','.join(f'{k}:{v!r}' for k, v in value.items())}}}")
    if isinstance(value, (list, tuple)):
        return Wrap(f"[{','.join(repr(wrap(v, blobs)) for v in value)}]")
    if isinstance(value, Wrap.nowrap()):
        return value

    raise TypeError(f'cannot wrap type {type(value)}')

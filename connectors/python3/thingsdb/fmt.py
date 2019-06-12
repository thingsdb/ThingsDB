from .model.thing import Thing


REPRS = (int, float, str, Thing)


def _wrap(value, blobs):

    if value is None:
        return 'nil'
    if isinstance(value, bool):
        return 'true' if value else 'false'
    if isinstance(value, bytes):
        idx = len(blobs)
        blobs.append(value)
        return f'blob({idx})'
    if isinstance(value, dict):
        thing_id = value.get('#')
        if thing_id is None:
            thing = ','.join(
                f'{k}:{_wrap(v, blobs)}'
                for k, v in value.items()
            )
            return f"{{{thing}}}"
        return f't({thing_id})'
    if isinstance(value, (list, tuple)):
        return f"[{','.join(_wrap(v, blobs) for v in value)}]"
    if isinstance(value, REPRS):
        return repr(value)

    raise TypeError(f'cannot wrap type `{type(value)}`')


def fmt(val, blobs=None):
    if blobs is None:
        blobs = []
    return _wrap(val, blobs)

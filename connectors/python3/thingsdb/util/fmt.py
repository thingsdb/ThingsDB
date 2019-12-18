def _wrap(value, blobs):

    if value is None:
        return 'nil'
    if isinstance(value, bool):
        return 'true' if value else 'false'
    if isinstance(value, bytes):
        idx = len(blobs)
        name = f'blob{idx}'
        blobs[name] = value
        return name
    if isinstance(value, dict):
        thing_id = value.get('#')
        if thing_id is None:
            thing = ','.join(
                f'{k}:{_wrap(v, blobs)}'
                for k, v in value.items()
            )
            return f"{{{thing}}}"
        return f'#{thing_id}'
    if isinstance(value, (list, tuple)):
        return f"[{','.join(_wrap(v, blobs) for v in value)}]"

    return repr(value)


def fmt(val, blobs=None):
    if blobs is None:
        blobs = {}
    return _wrap(val, blobs)

def convert(arg):
    if isinstance(arg, dict):
        id = arg.get('#')
        if id is None or len(arg) == 1:
            return {
                k: convert(v) for k, v in arg.items()
            }
        return {'#': id}

    if isinstance(arg, set):
        return {convert(v) for v in arg}

    if isinstance(arg, (list, tuple)):
        return [convert(v) for v in arg]

    return arg
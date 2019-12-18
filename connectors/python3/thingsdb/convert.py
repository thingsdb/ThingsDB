def convert(arg):
    if isinstance(arg, dict):
        id = arg.get('#')
        return {
            k: convert(v) for k, v in arg.items()
        } if id is None else {'#': id}

    if isinstance(arg, set):
        return {convert(v) for v in arg}

    if isinstance(arg, (list, tuple)):
        return [convert(v) for v in arg]

    return arg

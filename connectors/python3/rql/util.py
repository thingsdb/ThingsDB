def iterpairs(*pairs):
    it = iter(pairs)
    return zip(it, it)


def join(*events):
    event = events[0]
    for e in events[1:]:
        if e._db != event._db:
            raise ValueError('all events must have the same target database')
        event.tasks += e.tasks
    return event

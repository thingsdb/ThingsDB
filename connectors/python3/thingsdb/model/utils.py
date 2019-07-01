from .thing import Thing
from .keys import ARRAY_OF, SET_OF, REQUIRED, OPTIONAL


def array_of(cls):
    return type('ArrayOf', (list, ), {ARRAY_OF: cls})


def set_of(cls):
    return type('SetOf', (set, ), {SET_OF: cls})


def required(cls):
    if isinstance(cls, Thing):
        alt = dict
    elif isinstance(cls, list):
        alt = tuple
    else:
        alt = cls
    return type('Required', (cls,), {REQUIRED: alt})


def optional(cls):
    return type('Optional', (cls,), {OPTIONAL: None})

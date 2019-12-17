import re
import functools
import logging


class PropTypes:

    @staticmethod
    def any_(v, klass, collection):
        if isinstance(v, dict):
            if '#' in v:
                return PropTypes.thing_(v, klass, collection)
            if '$' in v:
                return PropTypes.set_(v, nested=functools.partial(
                    klass=klass,
                    collection=collection
                ))
            if '*' in v:
                pattern = v['*']
                flags = 0
                if pattern.endswith('i'):
                    flags |= re.IGNORECASE
                    pattern = pattern[:-2]
                else:
                    pattern = pattern[1:-1]
                return re.compile(pattern, flags)
            if '!':
                msg = v['error_msg']
                # TODO : Return correct exception
                return Exception(msg)
            logging.warning(f'unhandled dict: {v}')
        return v

    @staticmethod
    def thing_(v, klass, collection, watch=False):
        if not isinstance(v, dict):
            raise TypeError(f'expecting type `dict`, got `{type(v)}`')

        thing_id = v.pop('#')
        thing = collection._things.get(thing_id)

        if thing is None:
            thing = klass(collection, thing_id)
            thing.__dict__.update(v)

        if watch and not thing:
            collection.add_pending(thing)

        return thing

    @staticmethod
    def str_(v):
        if not isinstance(v, str):
            raise TypeError(f'expecting type `str`, got `{type(v)}`')
        return v

    @staticmethod
    def utf8_(v):
        if not isinstance(v, str):
            raise TypeError(f'expecting type `str`, got `{type(v)}`')
        return v

    @staticmethod
    def bytes_(v):
        if not isinstance(v, bytes):
            raise TypeError(f'expecting type `bytes`, got `{type(v)}`')
        return v

    @staticmethod
    def raw_(v, _types=(str, bytes)):
        if not isinstance(v, _types):
            raise TypeError(f'expecting type `bytes`, got `{type(v)}`')
        return v

    @staticmethod
    def int_(v):
        if not isinstance(v, int):
            raise TypeError(f'expecting type `int`, got `{type(v)}`')
        return v

    @staticmethod
    def uint_(v):
        if not isinstance(v, int):
            raise TypeError(f'expecting type `int`, got `{type(v)}`')
        if v < 0:
            raise ValueError(f'expecting an integer value >= 0, got {v}')
        return v

    @staticmethod
    def pint_(v):
        if not isinstance(v, int):
            raise TypeError(f'expecting type `int`, got `{type(v)}`')
        if v <= 0:
            raise ValueError(f'expecting an integer value > 0, got {v}')
        return v

    @staticmethod
    def nint_(v):
        if not isinstance(v, int):
            raise TypeError(f'expecting type `int`, got `{type(v)}`')
        if v >= 0:
            raise ValueError(f'expecting an integer value < 0, got {v}')
        return v

    @staticmethod
    def array_(v, nested):
        if not isinstance(v, list):
            raise TypeError(f'expecting a `list`, got `{type(v)}`')
        return [nested(item) for item in v]

    @staticmethod
    def set_(v, nested):
        if not isinstance(v, dict):
            raise TypeError(f'expecting a `dict`, got `{type(v)}`')
        v = v['$']
        return {nested(item) for item in v}

    def nillable(v, func=None):
        return v if v is None else func(v)

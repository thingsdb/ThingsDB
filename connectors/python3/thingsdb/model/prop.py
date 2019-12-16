class PropTypes:

    @staticmethod
    def any_(v):
        return v

    @staticmethod
    def str_(v):
        if not isinstance(v, str):
            raise TypeError(f'expecting type `str`, got `{type(v)}`')
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
    def array_(v):
        if not isinstance(v, (list, tuple)):
            raise TypeError(f'expecting a `list` or `tuple`, got `{type(v)}`')
        return v

    @staticmethod
    def set_(v):
        if not isinstance(v, (list, tuple, set)):
            raise TypeError(
                f'expecting a `list`, `tuple` or `set`, got `{type(v)}`')
        return set(v)


class Prop:

    __slots__ = (
        'vconv',
        'nconv',
        'spec',
    )

    @staticmethod
    def get_conv(fname, is_nillable):
        func = getattr(PropTypes, f'{fname}_')
        noneType = type(None)
        if func:
            return func if is_nillable else \
                    def validate(v): \
                    return isinstance(v, noneType) or func(v)

    def __init__(self, spec, cb=None):
        self.vconv = cb
        self.nconv = None
        self.spec = spec

    def unpack(self):
        from .thing import Thing  # nopep8
        if calleble(self.vconv):
            return

        cb_type, self.vconv = self.types, lambda: True
        spec = self.spec
        is_nillable = False

        if spec.endswith('?'):
            is_nillable = True
            spec = spec[:-1]

        assert spec, 'an empty specification is not allowed'

        if spec.startswith('['):
            assert spec.endswith(']'), 'missing `]` in specification'
            self.nconv = self.get_conv('array', is_nillable)
            spec = spec[1:-1]
            is_nillable = spec.endswith('?')
            if is_nillable:
                spec = spec[:-1]

        elif spec.startswith('{'):
            assert spec.endswith('}'), 'missing `}` in specification'
            self.nconv = self.get_conv('set', is_nillable)
            spec = spec[1:-1]
            is_nillable = False

        if not spec:
            self.vconv, self.nconv = self.nconv, self.get_conv('any', False)
            return  # finished, this is an array or set

        self.vconv = self.get_conv(spec, is_nillable)
        if self.vconv is None:
            assert calleble(cb_type), \
                f'type `{spec}` is not a build-in, a calleble is required'

            custum_type = cb_type()
            name = getattr(custum_type, '__NAME__', custum_type.__name__)
            assert spec == name, \
                f'type `{name}` does not match the specification `{spec}`'
            assert hasattr(custum_type, '_props'), \
                f'missing `_props`, type `{name}` ' \
                f'must be a subclass of `Thing`'
            for p in custum_type._props.values():
                p.unpack()
            self.vconv = def custom_type(v):
                if


        if spec == 'any':
            types.insert(0, object)
            lambda v: True
        elif spec == 'str' or spec == 'utf8':
            types.insert(0, str)
        elif spec == 'bytes':
            types.insert(0, bytes)
        elif spec == 'raw':
            types.insert(0, bytes)
            types.insert(0, str)
        elif (
            spec == 'int' or
            spec == 'uint' or
            spec == 'nint' or
            spec == 'pint'
        ):
            types.insert(0, int)
        elif spec == 'float':
            types.insert(0, float)
        elif spec == 'number':
            types.insert(0, int)
            types.insert(0, float)
        elif spec == 'bool':
            types.insert(0, bool)
        elif spec == 'thing':
            types.insert(0, dict)
        elif spec == 'Thing':
            types.insert(0, Thing)
        elif callable(cb_type):
            custum_type = cb_type()
            name = getattr(custum_type, '__NAME__', custum_type.__name__)
            assert spec == name, \
                f'type `{name}` does not match the specification `{spec}`'
            types.insert(0, custum_type)
            assert hasattr(custum_type, '_props'), \
                f'missing `_props`, type `{name}` ' \
                f'must be a subclass of `Thing`'
            for p in custum_type._props.values():
                p.unpack()
        else:
            assert 0, \
                f'type `{spec}` is not a build-in, a calleble is required'

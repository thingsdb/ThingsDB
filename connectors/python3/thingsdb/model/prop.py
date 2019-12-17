import functools
from .proptypes import PropTypes


class Prop:

    __slots__ = (
        'vconv',
        'nconv',
        'spec',
        'nillable'
    )

    @staticmethod
    def get_conv(fname, is_nillable=False, **kwrags):
        func = getattr(PropTypes, f'{fname}_', None)
        if kwrags:
            func = functools.partial(func, **kwrags)
        if func:
            return functools.partial(PropTypes.nillable, func=func) \
                if is_nillable else func

    def __init__(self, spec, cb=None):
        self.vconv = (cb, )
        self.nconv = None
        self.spec = spec
        self.nillable = False

    def unpack(self, collection):
        from .thing import Thing  # nopep8
        if callable(self.vconv):
            return

        cb_type, self.vconv = self.vconv[0], lambda: True
        spec = self.spec
        is_nillable = False

        if spec.endswith('?'):
            self.nillable = is_nillable = True
            spec = spec[:-1]

        assert spec, 'an empty specification is not allowed'

        if spec.startswith('['):
            assert spec.endswith(']'), 'missing `]` in specification'
            self.nconv = ('array', is_nillable)
            spec = spec[1:-1]
            is_nillable = spec.endswith('?')
            if is_nillable:
                spec = spec[:-1]

        elif spec.startswith('{'):
            assert spec.endswith('}'), 'missing `}` in specification'
            self.nconv = ('set', is_nillable)
            spec = spec[1:-1]
            is_nillable = False

        kwargs = {
            'klass': Thing,
            'collection': collection,
        }

        if not spec:
            nested = self.get_conv('any', **kwargs)
            self.vconv = self.get_conv(*self.nconv, nested=nested)
            self.nconv = nested
            return  # finished, this is an array or set

        self.vconv = self.get_conv(
            spec, is_nillable, **kwargs
            if spec == 'any' or spec == 'thing' else {})

        if self.vconv is None:
            assert callable(cb_type), \
                f'type `{spec}` is not a build-in, a calleble is required'

            custum_type = cb_type \
                if isinstance(cb_type, type) and issubclass(cb_type, Thing) \
                else cb_type()

            name = getattr(custum_type, '__NAME__', custum_type.__name__)

            assert spec == name, \
                f'type `{name}` does not match the specification `{spec}`'

            assert hasattr(custum_type, '_props'), \
                f'missing `_props`, type `{name}` ' \
                f'must be a subclass of `Thing`'

            kwargs['watch'] = {
                'klass': custum_type,
                'collection': collection,
                'watch': True,
            }
            self.vconv = self.get_conv('thing', is_nillable, **kwargs)

            for p in custum_type._props.values():
                p.unpack(collection)

        if self.nconv is not None:
            vconf = self.vconv
            self.vconv = self.get_conv(*self.nconv, nested=vconf)
            self.nconv = vconf

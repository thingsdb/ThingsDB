_TypeError = TypeError
_IndexError = IndexError
_RuntimeError = RuntimeError


class RqlError(Exception):
    pass


class AuthError(RqlError):
    pass


class NodeError(RqlError):
    pass


class TypeError(_TypeError, RqlError):
    pass


class IndexError(_IndexError, RqlError):
    pass


class RuntimeError(_RuntimeError, RqlError):
    pass

class RqlError(Exception):
    pass


class AuthError(RqlError):
    pass


class NodeError(RqlError):
    pass


class IndexError(RqlError):
    pass


class RuntimeError(RqlError):
    pass


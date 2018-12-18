_IndexError = IndexError
_ZeroDivisionError = ZeroDivisionError
_OverflowError = OverflowError


class ThingsDBError(Exception):
    def __init__(self, *args, errdata=None):
        if isinstance(errdata, dict):
            args = ('{error_msg} ({error_code})'.format_map(errdata), )
        super().__init__(*args)


class OverflowError(ThingsDBError, _OverflowError):
    pass


class ZeroDivisionError(ThingsDBError, _ZeroDivisionError):
    pass


class MaxQuotaError(ThingsDBError):
    pass


class AuthError(ThingsDBError):
    pass


class ForbiddenError(ThingsDBError):
    pass


class IndexError(ThingsDBError, _IndexError):
    pass


class BadRequestError(ThingsDBError):
    pass


class QueryError(ThingsDBError):
    pass


class NodeError(ThingsDBError):
    pass


class InternalError(ThingsDBError):
    pass

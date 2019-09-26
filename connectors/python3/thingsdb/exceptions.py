_AssertionError = AssertionError
_LookupError = LookupError
_MemoryError = MemoryError
_OverflowError = OverflowError
_ZeroDivisionError = ZeroDivisionError
_ValueError = ValueError
_TypeError = TypeError


class ThingsDBError(Exception):
    def __init__(self, *args, errdata=None):
        if isinstance(errdata, dict):
            args = (errdata['error_msg'], )
            self.error_code = errdata['error_code']
        super().__init__(*args)


class OperationError(ThingsDBError):
    pass


class NumArgumentsError(ThingsDBError):
    pass


class TypeError(ThingsDBError, _TypeError):
    pass


class ValueError(ThingsDBError, _ValueError):
    pass


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


class LookupError(ThingsDBError, _LookupError):
    pass


class BadDataError(ThingsDBError):
    pass


class SyntaxError(ThingsDBError):
    pass


class NodeError(ThingsDBError):
    pass


class AssertionError(ThingsDBError, _AssertionError):
    pass


class ResultTooLargeError(ThingsDBError):
    pass


class RequestTimeoutError(ThingsDBError):
    pass


class RequestCancelError(ThingsDBError):
    pass


class WriteUVError(ThingsDBError):
    pass


class MemoryError(ThingsDBError, _MemoryError):
    pass


class InternalError(ThingsDBError):
    pass

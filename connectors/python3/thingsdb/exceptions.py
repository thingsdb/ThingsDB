_IndexError = IndexError
_ZeroDivisionError = ZeroDivisionError
_OverflowError = OverflowError
_AssertionError = AssertionError


class ThingsDBError(Exception):
    def __init__(self, *args, errdata=None):
        if isinstance(errdata, dict):
            args = (errdata['error_msg'], )
            self.error_code = errdata['error_code']
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


class SyntaxError(ThingsDBError):
    pass


class NodeError(ThingsDBError):
    pass


class AssertionError(ThingsDBError, _AssertionError):
    pass


class InternalError(ThingsDBError):
    pass

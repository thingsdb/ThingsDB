import asyncio

_AssertionError = AssertionError
_LookupError = LookupError
_MemoryError = MemoryError
_OverflowError = OverflowError
_ZeroDivisionError = ZeroDivisionError
_ValueError = ValueError
_TypeError = TypeError
_CancelledError = asyncio.CancelledError


class TiException(Exception):
    pass


class CustomError(TiException):
    def __init__(self, err_code, err_msg):
        assert -100 >= err_code >= -127
        self.err_code = err_code
        super().__init__(err_msg)


class CancelledError(TiException, _CancelledError):
    err_code = -64


class OperationError(TiException):
    err_code = -63


class NumArgumentsError(TiException):
    err_code = -62


class TypeError(TiException, _TypeError):
    err_code = -61


class ValueError(TiException, _ValueError):
    err_code = -60


class OverflowError(TiException, _OverflowError):
    err_code = -59


class ZeroDivisionError(TiException, _ZeroDivisionError):
    err_code = -58


class MaxQuotaError(TiException):
    err_code = -57


class AuthError(TiException):
    err_code = -56


class ForbiddenError(TiException):
    err_code = -55


class LookupError(TiException, _LookupError):
    err_code = -54


class BadDataError(TiException):
    err_code = -53


class SyntaxError(TiException):
    err_code = -52


class NodeError(TiException):
    err_code = -51


class AssertionError(TiException, _AssertionError):
    err_code = -50


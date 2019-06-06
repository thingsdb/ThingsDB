package client

import qpack "github.com/transceptor-technology/go-qpack"

// ErrorCode known by ThingsDB
type ErrorCode int

const (
	// UnpackError - invalid qpack data when create a new Error
	UnpackError ErrorCode = -1

	/*
	 * Error below are a direct mapping to error codes from ThingsDB
	 * and are in the range from 96-127.
	 */

	// OverflowError - interger overflow
	OverflowError ErrorCode = ErrorCode(ProtoErrOverflow)
	// ZeroDivError - division or module by zero
	ZeroDivError ErrorCode = ErrorCode(ProtoErrZeroDiv)
	// MaxQuotaError - max quota is reached
	MaxQuotaError ErrorCode = ErrorCode(ProtoErrMaxQuota)
	// AuthError - authentication error
	AuthError ErrorCode = ErrorCode(ProtoErrAuth)
	// ForbiddenError - forbidden (access denied)
	ForbiddenError ErrorCode = ErrorCode(ProtoErrForbidden)
	// IndexError - requested resource not found
	IndexError ErrorCode = ErrorCode(ProtoErrIndex)
	// BadRequestError - unable to handle request due to invalid data
	BadRequestError ErrorCode = ErrorCode(ProtoErrBadRequest)
	// SyntaxError - syntax error in query
	SyntaxError ErrorCode = ErrorCode(ProtoErrSyntax)
	// NodeError - node is temporary unable to handle the request
	NodeError ErrorCode = ErrorCode(ProtoErrNode)
	// AssertionError - assertion statement has failed
	AssertionError ErrorCode = ErrorCode(ProtoErrAssertion)
	// InternalError - internal error
	InternalError ErrorCode = ErrorCode(ProtoErrInternal)
)

// Error can be returned by the siridb package.
type Error struct {
	msg  string
	code ErrorCode
}

// Err pointer
type Err *Error

// NewError returns a pointer to a new Error.
func NewError(msg string, code ErrorCode) Err {
	return &Error{
		msg:  msg,
		code: code,
	}
}

// NewErrorFromByte returns a pointer to a new Error from qpack byte data.
func NewErrorFromByte(b []byte) *Error {
	result, err := qpack.Unpack(b, qpack.QpFlagStringKeysOnly)
	if err != nil {
		return &Error{
			msg:  err.Error(),
			code: UnpackError,
		}
	}

	errMap, ok := result.(map[string]interface{})
	if !ok {
		return &Error{
			msg:  "expected a map",
			code: UnpackError,
		}
	}

	msg, ok := errMap["error_msg"].(string)
	if !ok {
		return &Error{
			msg:  "expected `error_msg` of type `string`",
			code: UnpackError,
		}
	}

	errCode, ok := errMap["error_code"].(int)
	if !ok {
		return &Error{
			msg:  "expected `error_code` of type `int`",
			code: UnpackError,
		}
	}

	return &Error{
		msg:  msg,
		code: ErrorCode(errCode),
	}
}

// Error returns the error msg.
func (e *Error) Error() string { return e.msg }

// Code returns the error type.
func (e *Error) Code() ErrorCode { return e.code }

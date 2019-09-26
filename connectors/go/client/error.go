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

	// OperationError - operation is not valid in the current context
	OperationError ErrorCode = ErrorCode(-63)
	// NumArgumentsError - wrong number of arguments
	NumArgumentsError ErrorCode = ErrorCode(-62)
	// TypeError - object of inappropriate type
	TypeError ErrorCode = ErrorCode(-61)
	// ValueError - object has the right type but an inappropriate value
	ValueError ErrorCode = ErrorCode(-60)
	// OverflowError - interger overflow
	OverflowError ErrorCode = ErrorCode(-59)
	// ZeroDivError - division or module by zero
	ZeroDivError ErrorCode = ErrorCode(-58)
	// MaxQuotaError - max quota is reached
	MaxQuotaError ErrorCode = ErrorCode(-57)
	// AuthError - authentication error
	AuthError ErrorCode = ErrorCode(-56)
	// ForbiddenError - forbidden (access denied)
	ForbiddenError ErrorCode = ErrorCode(-55)
	// LookupError - requested resource not found
	LookupError ErrorCode = ErrorCode(-54)
	// BadRequestError - unable to handle request due to invalid data
	BadRequestError ErrorCode = ErrorCode(-53)
	// SyntaxError - syntax error in query
	SyntaxError ErrorCode = ErrorCode(-52)
	// NodeError - node is temporary unable to handle the request
	NodeError ErrorCode = ErrorCode(-51)
	// AssertionError - assertion statement has failed
	AssertionError ErrorCode = ErrorCode(-50)

	// ResultTooLargeError - result too large
	ResultTooLargeError = ErrorCode(-6)
	// RequestTimeoutError - request timed out
	RequestTimeoutError = ErrorCode(-5)
	// RequestCancelError - request is cancelled
	RequestCancelError = ErrorCode(-4)
	// WriteUVError - cannot write to socket
	WriteUVError ErrorCode = ErrorCode(-3)
	// MemoryError - memory allocation error
	MemoryError ErrorCode = ErrorCode(-2)
	// InternalError - internal error
	InternalError ErrorCode = ErrorCode(-1)
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

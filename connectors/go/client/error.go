package client

import qpack "github.com/transceptor-technology/go-qpack"

// ErrorCode known by ThingsDB
type ErrorCode int

const (
	// UnpackError - invalid qpack data when create a new Error
	UnpackError ErrorCode = -1
	// OverflowError - interger overflow
	OverflowError ErrorCode = 96
)

// Error can be returned by the siridb package.
type Error struct {
	msg  string
	code ErrorCode
}

// NewError returns a pointer to a new Error.
func NewError(msg string, code ErrorCode) *Error {
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
	errMap := result.(map[string]interface{})
	return &Error{
		msg:  errMap["error_msg"].(string),
		code: errMap["error_code"].(ErrorCode),
	}
}

// Error returns the error msg.
func (e *Error) Error() string { return e.msg }

// Code returns the error type.
func (e *Error) Code() ErrorCode { return e.code }

package client

import qpack "github.com/transceptor-technology/go-qpack"

// ErrorCode known by ThingsDB
type ErrorCode int

const (
	// UnpackError when no error can be created from qpack data.
	UnpackError   ErrorCode = -1
	OverflowError ErrorCode = 96
)

// Error can be returned by the siridb package.
type Error struct {
	msg  string
	code int
}

// NewError returns a pointer to a new Error.
func NewError(msg string, code int) *Error {
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
		code: errMap["error_code"].(int),
	}
}

// Msg returns the error msg.
func (e *Error) Msg() string { return e.msg }

// Code returns the error code.
func (e *Error) Code() uint8 { return e.code }

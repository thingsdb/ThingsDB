package client

// Proto is used as protocol type used by ThingsDB.
type Proto int8

const (
	/*
	 * Requests
	 */

	// ProtoReqPing requires `nil`
	ProtoReqPing Proto = 32
	// ProtoReqAuth requires `[username, password]`
	ProtoReqAuth Proto = 33
	// ProtoReqQuery requires `[scope, query [, arguments]]`
	ProtoReqQuery Proto = 34

	/*
	 * Responses
	 */

	// ProtoResPing responds with `nil`
	ProtoResPing Proto = 64
	// ProtoResAuth responds with `nil`
	ProtoResAuth Proto = 65
	// ProtoResQuery responds with `[{..}, {..}...] or {...}`
	ProtoResQuery Proto = 66

	/*
	 * Errors `{error_msg: ..., error_code: ...}`
	 */

	// ProtoErrOperation - operation is not valid in the current context
	ProtoErrOperation Proto = -63
	// ProtoErrNumArguments - wrong number of arguments
	ProtoErrNumArguments Proto = -62
	// ProtoErrType - object of inappropriate type
	ProtoErrType Proto = -61
	// ProtoErrValue - object has the right type but an inappropriate value
	ProtoErrValue Proto = -60
	// ProtoErrOverflow - integer overflow
	ProtoErrOverflow Proto = -59
	// ProtoErrZeroDiv - division or module by zero
	ProtoErrZeroDiv Proto = -58
	// ProtoErrMaxQuota - max quota is reached
	ProtoErrMaxQuota Proto = -57
	// ProtoErrAuth - authentication error
	ProtoErrAuth Proto = -56
	// ProtoErrForbidden - forbidden (access denied)
	ProtoErrForbidden Proto = -55
	// ProtoErrLookup - requested resource not found
	ProtoErrLookup Proto = -54
	// ProtoErrBadRequest - unable to handle request due to invalid data
	ProtoErrBadRequest Proto = -53
	// ProtoErrSyntax - syntax error in query
	ProtoErrSyntax Proto = -52
	// ProtoErrNode - node is temporary unable to handle the request
	ProtoErrNode Proto = -51
	// ProtoErrAssertion - assertion statement has failed
	ProtoErrAssertion Proto = -50

	// ProtoErrRequestTimeout - request timed out
	ProtoErrRequestTimeout Proto = -5
	// ProtoErrRequestCancel - request is cancelled
	ProtoErrRequestCancel Proto = -4
	// ProtoErrWriteUV - cannot write to socket
	ProtoErrWriteUV Proto = -3
	// ProtoErrMemory - memory allocation error
	ProtoErrMemory Proto = -2
	// ProtoErrInternal - internal error
	ProtoErrInternal Proto = -1
)

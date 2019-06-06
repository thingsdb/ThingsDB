package client

// Proto is used as protocol type used by ThingsDB.
type Proto uint8

const (
	/*
	 * Requests
	 */

	// ProtoReqPing requires `nil`
	ProtoReqPing Proto = 32
	// ProtoReqAuth requires `[username, password]`
	ProtoReqAuth Proto = 33
	// ProtoReqQueryNode requires `{query: ...}`
	ProtoReqQueryNode Proto = 34
	// ProtoReqQueryThingsDB requires `{query: ...}`
	ProtoReqQueryThingsDB Proto = 35
	// ProtoReqQueryCollection requires `{collection: ..., query: ...}`
	ProtoReqQueryCollection Proto = 36

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

	// ProtoErrOverflow -integer overflow
	ProtoErrOverflow Proto = 96
	// ProtoErrZeroDiv - division or module by zero
	ProtoErrZeroDiv Proto = 97
	// ProtoErrMaxQuota - max quota is reached
	ProtoErrMaxQuota Proto = 98
	// ProtoErrAuth - authentication error
	ProtoErrAuth Proto = 99
	// ProtoErrForbidden - forbidden (access denied)
	ProtoErrForbidden Proto = 100
	// ProtoErrIndex - requested resource not found
	ProtoErrIndex Proto = 101
	// ProtoErrBadRequest - unable to handle request due to invalid data
	ProtoErrBadRequest Proto = 102
	// ProtoErrSyntax - syntax error in query
	ProtoErrSyntax Proto = 103
	// ProtoErrNode - node is temporary unable to handle the request
	ProtoErrNode Proto = 104
	// ProtoErrAssertion - assertion statement has failed
	ProtoErrAssertion Proto = 105
	// ProtoErrInternal - internal error
	ProtoErrInternal Proto = 127
)

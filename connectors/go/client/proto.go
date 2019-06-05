package client

// Proto is used as protocol type used by ThingsDB.
type Proto int

const (
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

	// ProtoResPing responds with `nil`
	ProtoResPing Proto = 64
	// ProtoResAuth responds with `nil`
	ProtoResAuth Proto = 65
	// ProtoResQuery responds with `[{..}, {..}...] or {...}`
	ProtoResQuery Proto = 66

	// ProtoResQuery responds with `{error_msg: `
	ProtoErrOverflow Proto = 96
	TI_PROTO_CLIENT_ERR_OVERFLOW
)

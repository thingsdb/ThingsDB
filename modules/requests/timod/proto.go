package timod

// Proto is used as protocol type used by ThingsDB.
type Proto int8

const (
	// ProtoModuleInit initializes extension
	ProtoModuleInit Proto = 64

	// ProtoModuleReq when a request from ThingsDB is received
	ProtoModuleReq Proto = 65

	// ProtoModuleRes is used to respond to a ProtoModuleReq package
	ProtoModuleRes Proto = 66

	// ProtoModuleErr is used to respond with an error
	ProtoModuleErr Proto = 67
)

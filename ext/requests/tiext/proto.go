package tiext

// Proto is used as protocol type used by ThingsDB.
type Proto int8

const (
	// ProtoExtInit initializes extension
	ProtoExtInit Proto = 64

	// ProtoExtClose for triggering to close the extension
	ProtoExtClose Proto = 65

	// ProtoExtReq when a request from ThingsDB is received
	ProtoExtReq Proto = 66

	// ProtoExtRes is used to respond to a ProtoExtReq package
	ProtoExtRes Proto = 67
)

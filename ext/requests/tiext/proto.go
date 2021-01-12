package tiext

// Proto is used as protocol type used by ThingsDB.
type Proto int8

const (

	// ProtoExtInit initializes extension
	ProtoExtInit Proto = 64

	// ProtoExtClose for triggering to close the extension
	ProtoExtClose Proto = 65
)

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
	 * Events
	 */

	// ProtoOnWatchIni initial data for a `thing`
	ProtoOnWatchIni Proto = 16

	// ProtoOnWatchUpd initial update data for a `thing`
	ProtoOnWatchUpd Proto = 17

	// ProtoOnWatchDel `thing` is removed from ThingsDB
	ProtoOnWatchDel Proto = 18

	// ProtoOnNodeStatus the connected node has changed it's status
	ProtoOnNodeStatus Proto = 19

	// ProtoOnWarn warning message for the connected client
	ProtoOnWarn Proto = 20

	/*
	 * Responses
	 */

	// ProtoResPing responds with `nil`
	ProtoResPing Proto = 64
	// ProtoResAuth responds with `nil`
	ProtoResAuth Proto = 65
	// ProtoResQuery responds with `...`
	ProtoResQuery Proto = 66
	// ProtoResError responds with `{error_msg:..., error_code:,...}`
	ProtoResError Proto = 96
)

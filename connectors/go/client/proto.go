package client

// Proto is used as protocol type used by ThingsDB.
type Proto int8

const (

	/*
	 * Events
	 */

	// ProtoOnNodeStatus the connected node has changed it's status
	ProtoOnNodeStatus Proto = 0

	// ProtoOnWatchIni initial data for a `thing`
	ProtoOnWatchIni Proto = 1

	// ProtoOnWatchUpd initial update data for a `thing`
	ProtoOnWatchUpd Proto = 2

	// ProtoOnWatchDel `thing` is removed from ThingsDB
	ProtoOnWatchDel Proto = 3

	// ProtoOnWarn warning message for the connected client
	ProtoOnWarn Proto = 4

	/*
	 * Responses
	 */

	// ProtoResPing responds with `nil`
	ProtoResPing Proto = 16
	// ProtoResAuth responds with `nil`
	ProtoResAuth Proto = 17
	// ProtoResQuery responds with `...`
	ProtoResQuery Proto = 18
	// ProtoResWatch responds with `nil`
	ProtoResWatch Proto = 19
	// ProtoResUnWatch responds with `nil`
	ProtoResUnWatch Proto = 20
	// ProtoResError responds with `{error_msg:..., error_code:,...}`
	ProtoResError Proto = 21

	/*
	 * Requests
	 */

	// ProtoReqPing requires `nil`
	ProtoReqPing Proto = 32
	// ProtoReqAuth requires `[username, password]`
	ProtoReqAuth Proto = 33
	// ProtoReqQuery requires `[scope, query [, arguments]]`
	ProtoReqQuery Proto = 34
	//[scope, thing id's....]}
	ProtoReqWatch Proto = 35
	//[scope, thing id's....]}
	ProtoReqUnwatch Proto = 36
	//[scope, procedure, arguments....]}
	ProtoReqRun Proto = 37
)

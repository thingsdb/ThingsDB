package client

// Req interface for requests.
type Req interface {
	GetTimeout() uint16
	SetTimeout(timeout uint16)
	GetProtocol() Proto
	AsMap() map[string]interface{}
}

type req struct {
	protocol Proto
	query    string
	timeout  uint16
}

// ReqThingsDB for constructing a ThingsDB query.
type ReqThingsDB struct {
	req
	All bool
}

// NewReqThingsDB for constructing a ThingsDB query.
func NewReqThingsDB(query string) *ReqThingsDB {
	return &ReqThingsDB{
		req: req{
			protocol: ProtoReqQueryThingsDB,
			query:    query,
			timeout:  10,
		},
		All: false,
	}
}

// AsMap exports the request map
func (r *ReqThingsDB) AsMap() map[string]interface{} {
	return map[string]interface{}{
		"query": r.query,
	}
}

// GetTimeout to set the request timeout in seconds
func (r *req) GetTimeout() uint16 {
	return r.timeout
}

// SetTimeout to set the request timeout in seconds
func (r *req) SetTimeout(timeout uint16) {
	r.timeout = timeout
}

// GetProtocol to set the request timeout in seconds
func (r *req) GetProtocol() Proto {
	return r.protocol
}

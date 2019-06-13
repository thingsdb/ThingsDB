package client

// Req interface for requests.
type Req interface {
	GetTimeout() uint16
	SetTimeout(timeout uint16)
	GetProtocol() Proto
	AsMap() map[string]interface{}
}

type request struct {
	protocol Proto
	query    string
	timeout  uint16
}

// ReqNode for constructing a Node query.
type ReqNode struct {
	request
	All bool
}

// ReqThingsDB for constructing a ThingsDB query.
type ReqThingsDB struct {
	request
	All bool
}

// ReqCollection for constructing a Collection query.
type ReqCollection struct {
	request
	collection string
	All        bool
	Deep       uint8
	Blobs      []byte
}

// NewReqNode for constructing a Node query.
func NewReqNode(query string) *ReqNode {
	return &ReqNode{
		request: request{
			protocol: ProtoReqQueryNode,
			query:    query,
			timeout:  10,
		},
		All: false,
	}
}

// NewReqThingsDB for constructing a ThingsDB query.
func NewReqThingsDB(query string) *ReqThingsDB {
	return &ReqThingsDB{
		request: request{
			protocol: ProtoReqQueryThingsDB,
			query:    query,
			timeout:  10,
		},
		All: false,
	}
}

// NewReqCollection for constructing a Collection query.
func NewReqCollection(query, collection string, deep uint8) *ReqCollection {
	return &ReqCollection{
		request: request{
			protocol: ProtoReqQueryCollection,
			query:    query,
			timeout:  10,
		},
		collection: collection,
		All:        false,
		Deep:       deep,
	}
}

// AsMap exports the request map
func (r *ReqNode) AsMap() map[string]interface{} {
	m := map[string]interface{}{
		"query": r.query,
	}
	if r.All {
		m["all"] = true
	}
	return m
}

// AsMap exports the request map
func (r *ReqThingsDB) AsMap() map[string]interface{} {
	m := map[string]interface{}{
		"query": r.query,
	}
	if r.All {
		m["all"] = true
	}
	return m
}

// AsMap exports the request map
func (r *ReqCollection) AsMap() map[string]interface{} {
	m := map[string]interface{}{
		"query":      r.query,
		"collection": r.collection,
		"deep":       r.Deep,
	}
	if r.All {
		m["all"] = true
	}
	if len(r.Blobs) > 0 {
		m["blobs"] = r.Blobs
	}
	return m
}

// GetTimeout to set the request timeout in seconds
func (r *request) GetTimeout() uint16 {
	return r.timeout
}

// SetTimeout to set the request timeout in seconds
func (r *request) SetTimeout(timeout uint16) {
	r.timeout = timeout
}

// GetProtocol to set the request timeout in seconds
func (r *request) GetProtocol() Proto {
	return r.protocol
}

package client

// Scope is used for node, thingsdb or a collection scope.
type Scope struct {
	protocol   Proto
	collection string
}

// NewScope return a
func NewScope(collection string) *Scope {
	return &Scope{
		protocol:   ProtoReqQueryCollection,
		collection: collection,
	}
}

// ScopeNode is the scope to use when querying the ThingsDB scope.
const ScopeNode = Scope{
	protocol:   ProtoReqQueryNode,
	collection: nil,
}

// ScopeThingsDB is the scope to use when querying the ThingsDB scope.
const ScopeThingsDB = Scope{
	protocol:   ProtoReqQueryThingsDB,
	collection: nil,
}

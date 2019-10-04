package client

import "github.com/transceptor-technology/go-qpack"

// Event is writting to the client event channel in case of an event
type Event struct {
	proto Proto
	data  interface{}
}

// newEvent creates a new event
func newEvent(pkg *pkg) (*Event, error) {
	result, err := qpack.Unpack(pkg.data, qpack.QpFlagStringKeysOnly)
	if err != nil {
		return nil, err
	}

	return &Event{
		proto: Proto(pkg.tp),
		data:  result,
	}, nil
}

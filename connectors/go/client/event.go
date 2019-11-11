package client

import (
	"gopkg.in/vmihailenco/msgpack.v4"
)

// Event is writting to the client event channel in case of an event
type Event struct {
	Proto Proto
	Data  interface{}
}

// newEvent creates a new event
func newEvent(pkg *pkg) (*Event, error) {
	var result interface{}
	err := msgpack.Unmarshal(pkg.data, &result)
	if err != nil {
		return nil, err
	}

	return &Event{
		Proto: Proto(pkg.tp),
		Data:  result,
	}, nil
}

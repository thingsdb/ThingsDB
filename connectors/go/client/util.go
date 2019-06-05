package client

import "io"

func niceErr(err error) string {
	if err == io.EOF {
		return "connection lost"
	}
	return err.Error()
}

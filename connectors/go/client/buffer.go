package client

import (
	"net"
)

// buffer is used to read data from a connection.
type buffer struct {
	data  []byte
	len   uint32
	pkg   *pkg
	conn  net.Conn
	pkgCh chan *pkg
	evCh  chan *pkg
	errCh chan error
}

// newBuffer retur a pointer to a new buffer.
func newBuffer() *buffer {
	return &buffer{
		data:  make([]byte, 0),
		len:   0,
		pkg:   nil,
		conn:  nil,
		pkgCh: make(chan *pkg),
		evCh:  make(chan *pkg),
		errCh: make(chan error, 1),
	}
}

// read listens on a connection for data.
func (buf buffer) read() {
	for {
		// try to read the data
		wbuf := make([]byte, 8192)
		n, err := buf.conn.Read(wbuf)

		if err != nil {
			// send an error if it's encountered
			buf.errCh <- err
			return
		}

		buf.len += uint32(n)
		buf.data = append(buf.data, wbuf[:n]...)

		for buf.len >= pkgHeaderSize {
			if buf.pkg == nil {
				buf.pkg, err = newPkg(buf.data)
				if err != nil {
					buf.errCh <- err
					return
				}
			}

			total := buf.pkg.size + pkgHeaderSize

			if buf.len < total {
				break
			}

			buf.pkg.setData(&buf.data, total)

			// The reserved ThingsDB events range is between 16..31
			if buf.pkg.tp >= 16 && buf.pkg.tp <= 31 {
				buf.evCh <- buf.pkg
			} else {
				buf.pkgCh <- buf.pkg
			}

			buf.data = buf.data[total:]
			buf.len -= total
			buf.pkg = nil
		}
	}
}

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
	errCh chan error
}

// newBuffer retur a pointer to a new buffer.
func newBuffer() *buffer {
	return &buffer{
		buf:   make([]byte, 0),
		len:   0,
		pkg:   nil,
		conn:  nil,
		pkgCh: make(chan *Pkg),
		errCh: make(chan error, 1),
	}
}

// Read listens on a connection for data.
func (buf buffer) read() {
	for {
		// try to read the data
		wbuf := make([]byte, 8192)
		n, err := buf.conn.Read(buf)

		if err != nil {
			// send an error if it's encountered
			buf.errCh <- err
			return
		}

		wbuf.len += uint32(n)
		buf.data = append(buf.data, wbuf[:n]...)

		for buf.len >= pkgHeaderSize {
			if buf.pkg == nil {
				buf.pkg, err = newPkg(buf.data)
				if err != nil {
					buffer.errCh <- err
					return
				}
			}

			total := buffer.pkg.size + pkgHeaderSize

			if buffer.len < total {
				break
			}

			buf.pkg.setData(&buf.data, total)

			buf.pkgCh <- buf.pkg

			buf.data = buf.data[total:]
			buf.len -= total
			buf.pkg = nil
		}
	}
}

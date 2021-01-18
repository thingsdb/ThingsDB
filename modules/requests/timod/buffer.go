package timod

import (
	"bufio"
	"os"
)

// Buffer is used to read data from a connection.
type Buffer struct {
	data   []byte
	len    uint32
	pkg    *Pkg
	reader *bufio.Reader
	PkgCh  chan *Pkg
	ErrCh  chan error
}

// NewBuffer retur a pointer to a new buffer.
func NewBuffer() *Buffer {
	return &Buffer{
		data:   make([]byte, 0),
		len:    0,
		pkg:    nil,
		reader: bufio.NewReader(os.Stdin),
		PkgCh:  make(chan *Pkg),
		ErrCh:  make(chan error, 1),
	}
}

// Listen listens on a connection for data.
func (buf Buffer) Listen() {
	for {
		// try to read the data
		wbuf := make([]byte, 4)
		n, err := buf.reader.Read(wbuf)

		if err != nil {
			// send an error if it's encountered
			buf.ErrCh <- err
			return
		}

		buf.len += uint32(n)
		buf.data = append(buf.data, wbuf[:n]...)

		for buf.len >= pkgHeaderSize {
			if buf.pkg == nil {
				buf.pkg, err = newPkg(buf.data)
				if err != nil {
					buf.ErrCh <- err
					return
				}
			}

			total := buf.pkg.Size + pkgHeaderSize

			if buf.len < total {
				break
			}

			buf.pkg.setData(&buf.data, total)

			buf.PkgCh <- buf.pkg

			buf.data = buf.data[total:]
			buf.len -= total
			buf.pkg = nil
		}
	}
}

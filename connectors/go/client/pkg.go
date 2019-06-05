package client

import (
	"encoding/binary"
	"fmt"

	"github.com/transceptor-technology/go-qpack"
)

// pkgHeaderSize is the size of a package header.
const pkgHeaderSize = 8

// PkgInitCapacity will be used as default capacity when allocating for a package.
const pkgInitCapacity = 8192

// pkg contains of a header and data.
type pkg struct {
	size uint32
	pid  uint16
	tp   uint8
	data []byte
}

// newPkg returns a poiter to a new pkg.
func newPkg(b []byte) (*pkg, error) {
	tp := b[6]
	check := b[7]

	if check != '\xff'^tp {
		return nil, fmt.Errorf("invalid checkbit")
	}

	return &pkg{
		size: binary.LittleEndian.Uint32(b),
		pid:  binary.LittleEndian.Uint16(b[4:]),
		tp:   tp,
		data: nil,
	}, nil
}

// setData sets package data
func (p *pkg) setData(b *[]byte, size uint32) {
	p.data = (*b)[pkgHeaderSize:size]
}

// pack returns a byte array containing a header with serialized data.
func pkgPack(pid uint16, tp uint8, v interface{}) ([]byte, error) {
	var err error
	var data []byte

	pkg := make([]byte, pkgHeaderSize, PkgInitCapacity)

	if v != nil {
		data, err = qpack.PackTo(pkg, v)
		if err != nil {
			return nil, err
		}
	}

	// set package length.
	binary.LittleEndian.PutUint32(pkg[0:], uint32(len(data)-pkgHeaderSize))

	// set package pid.
	binary.LittleEndian.PutUint16(pkg[4:], pid)

	// set package type and check bit.
	pkg[6] = tp
	pkg[7] = '\xff' ^ tp

	return pkg, nil
}

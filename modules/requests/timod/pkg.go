package timod

import (
	"encoding/binary"
	"fmt"

	"github.com/vmihailenco/msgpack/v4"
)

// pkgHeaderSize is the size of a package header.
const pkgHeaderSize = 8

// PkgInitCapacity will be used as default capacity when allocating for a package.
const pkgInitCapacity = 8192

// pkg contains of a header and data.
type pkg struct {
	Size uint32
	Pid  uint16
	Tp   Proto
	Data []byte
}

// newPkg returns a poiter to a new pkg.
func newPkg(b []byte) (*pkg, error) {
	tp := b[6]
	check := b[7]

	if check != '\xff'^tp {
		return nil, fmt.Errorf("invalid checkbit")
	}

	return &pkg{
		Size: binary.LittleEndian.Uint32(b),
		Pid:  binary.LittleEndian.Uint16(b[4:]),
		Tp:   Proto(tp),
		Data: nil,
	}, nil
}

// setData sets package data
func (p *pkg) setData(b *[]byte, size uint32) {
	p.Data = (*b)[pkgHeaderSize:size]
}

// PkgPackBin returns a byte array containing a header with serialized data.
func PkgPackBin(pid uint16, tp Proto, data []byte) []byte {

	datasz := len(data)

	pkgdata := make([]byte, pkgHeaderSize, pkgHeaderSize+datasz)
	pkgdata = append(pkgdata, data...)

	// set package length.
	binary.LittleEndian.PutUint32(pkgdata[0:], uint32(datasz))

	// set package pid.
	binary.LittleEndian.PutUint16(pkgdata[4:], pid)

	// set package type and check bit.
	pkgdata[6] = uint8(tp)
	pkgdata[7] = '\xff' ^ uint8(tp)

	return pkgdata
}

// PkgPack returns a byte array containing a header with serialized data.
func PkgPack(pid uint16, tp Proto, v interface{}) ([]byte, error) {

	data, err := msgpack.Marshal(v)
	if err != nil {
		return nil, err
	}

	data = PkgPackBin(pid, tp, data)

	return data, nil
}

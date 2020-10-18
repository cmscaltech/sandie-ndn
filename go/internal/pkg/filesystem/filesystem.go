// Copyright (C) 2020 California Institute of Technology
// Author: Catalin Iordache <catalin.iordache@cern.ch>

package filesystem

import (
	"bytes"
	"encoding/binary"
)

type FileInfo struct {
	Size int64
}

func MarshalFileInfo(f FileInfo) ([]byte, error) {
	buf := new(bytes.Buffer)

	if e := binary.Write(buf, binary.BigEndian, &f); e != nil {
		return nil, e
	}
	return buf.Bytes(), nil
}

func UnmarshalFileInfo(value []byte) (f FileInfo, e error) {
	e = binary.Read(bytes.NewReader(value), binary.BigEndian, &f)
	return f, e
}


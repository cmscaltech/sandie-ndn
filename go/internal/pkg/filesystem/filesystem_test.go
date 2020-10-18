// Copyright (C) 2020 California Institute of Technology
// Author: Catalin Iordache <catalin.iordache@cern.ch>

package filesystem_test

import (
	"github.com/cmscaltech/sandie-ndn/go/internal/pkg/filesystem"
	"github.com/usnistgov/ndn-dpdk/ndn"
	"testing"
)

func TestGetFilePath(t *testing.T) {
	assert, _ := makeAR(t)

	fileInfo := filesystem.FileInfo{
		Size: 1024,
	}

	payload, e := filesystem.MarshalFileInfo(fileInfo)
	assert.NoError(e)

	data := ndn.MakeData(ndn.ParseName("/A/B/C/F"), payload)
	fileInfo, e = filesystem.UnmarshalFileInfo(data.Content)
	assert.NoError(e)

	assert.EqualValues(1024, fileInfo.Size)
}
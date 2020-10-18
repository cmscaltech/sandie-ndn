// Copyright (C) 2020 California Institute of Technology
// Author: Catalin Iordache <catalin.iordache@cern.ch>

package utils_test

import (
	"github.com/cmscaltech/sandie-ndn/go/internal/pkg/utils"
	"github.com/usnistgov/ndn-dpdk/ndn"
	"testing"
)

func TestGetFilePath(t *testing.T) {
	assert, _ := makeAR(t)

	name := ndn.ParseName("/A/B/C/F")
	assert.EqualValues("/A/B/C/F", utils.GetFilePath(name))
}

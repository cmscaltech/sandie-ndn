// Copyright (C) 2020 California Institute of Technology
// Author: Catalin Iordache <catalin.iordache@cern.ch>

package utils

import (
	"fmt"
	"github.com/usnistgov/ndn-dpdk/ndn"
	"github.com/usnistgov/ndn-dpdk/ndn/an"
)

func GetFilePath(name ndn.Name) string {
	var slice []byte
	for _, comp := range name {
		if comp.Type != uint32(an.TtGenericNameComponent) {
			panic(fmt.Errorf("invalid file path name component type"))
		}

		slice = append(slice, '/')
		slice = append(slice, comp.Value...)
	}

	return string(slice)
}

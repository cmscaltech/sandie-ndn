// Copyright (C) 2020 California Institute of Technology
// Author: Catalin Iordache <catalin.iordache@cern.ch>

package utils

import "github.com/usnistgov/ndn-dpdk/ndn/tlv"

func ReadNNI(wire []byte) (value tlv.NNI, e error) {
	if e = value.UnmarshalBinary(wire); e != nil {
		return 0, e
	}
	return value, nil
}

// Copyright (C) 2020 California Institute of Technology
// Author: Catalin Iordache <catalin.iordache@cern.ch>

package pipeline

import (
	"github.com/usnistgov/ndn-dpdk/ndn"
	"github.com/usnistgov/ndn-dpdk/ndn/l3"
)

type Pipeline interface {
	Face() l3.Face

	Send() chan <- ndn.Name
	OnData() <- chan *ndn.Data
	OnFailure() <- chan error

	Close()
}

// Copyright (C) 2020 California Institute of Technology
// Author: Catalin Iordache <catalin.iordache@cern.ch>

package consumer

import (
	"github.com/usnistgov/ndn-dpdk/ndn/packettransport/afpacket"
)

// Config
type Config struct {
	Ifname string
	Gqluri string
	afpacket.Config
}

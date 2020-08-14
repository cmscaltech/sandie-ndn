// Copyright (C) 2020 California Institute of Technology
// Author: Catalin Iordache <catalin.iordache@cern.ch>

package consumer

import (
	"github.com/usnistgov/ndn-dpdk/ndn/packettransport/afpacket"
)

// Settings
type Settings struct {
	Ifname string
	Gqluri string
	afpacket.Config
}

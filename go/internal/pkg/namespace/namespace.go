// Copyright (C) 2020 California Institute of Technology
// Author: Catalin Iordache <catalin.iordache@cern.ch>

package namespace

import "time"

const (
	ESuccess, EFailure = 0, 1
)

const (
	// NamePrefixUri Name prefix for all packets
	NamePrefixUri = "/ndn/xrootd"

	// NamePrefixUriFileinfo Name prefix for all packets addressing FileInfo Data
	NamePrefixUriFileinfo = NamePrefixUri + "/fileinfo"

	// NamePrefixUriFileRead Name prefix for all packets addressing Read Data
	NamePrefixUriFileRead = NamePrefixUri + "/read"
)

const (
	// MaxPayloadSize Maximum payload size inside Data packets
	MaxPayloadSize = 6144

	// DefaultInterestLifetime Interestlifetime
	DefaultInterestLifetime = 8 * time.Second
)

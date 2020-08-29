// Copyright (C) 2020 California Institute of Technology
// Author: Catalin Iordache <catalin.iordache@cern.ch>

package pipeline

import (
	"github.com/usnistgov/ndn-dpdk/ndn"
	"github.com/usnistgov/ndn-dpdk/ndn/l3"
	"time"
)

type Pipeline interface {
	Face() l3.Face

	Send() chan<- ndn.Interest
	OnData() <-chan *ndn.Data
	OnFailure() <-chan error
	Close()
}

var (
	MaxNackCongestion        = 4
	MaxCongestionBackoffTime = time.Second * 2
	MaxTimeoutRetries        = 4
)

type Entry struct {
	interest        ndn.Interest
	nNackCongestion int
	nNackDuplicate  int
	nTimeouts       int
}

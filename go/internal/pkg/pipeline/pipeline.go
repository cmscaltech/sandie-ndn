// Copyright (C) 2020 California Institute of Technology
// Author: Catalin Iordache <catalin.iordache@cern.ch>

package pipeline

import (
	"github.com/usnistgov/ndn-dpdk/ndn"
	"time"
)

// Pipeline represents the congestion and retransmission logic of packets
// expressed by one consumer
type Pipeline interface {
	// Send returns a channel on which the consumer can express Interest packets
	Send() chan<- ndn.Interest

	// OnData returns a channel on which Data packets are sent back
	OnData() <-chan *ndn.Data

	// OnFailure returns a channel to raise errors that might occur during packet
	// handling over NDN such as: maximum number of timeouts has been exceed, maximum
	// number of congestion NACKs has been exceeded
	OnFailure() <-chan error

	Close()
}

// Entry represents one entry in the pipeline. Used for retransmission logic
type Entry struct {
	interest        ndn.Interest
	nNackCongestion int
	nNackDuplicate  int
	nTimeouts       int
}

var (
	MaxNackCongestion        = 4
	MaxCongestionBackoffTime = time.Second * 2
	MaxTimeoutRetries        = 4
)
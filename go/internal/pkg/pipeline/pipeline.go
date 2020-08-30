// Copyright (C) 2020 California Institute of Technology
// Author: Catalin Iordache <catalin.iordache@cern.ch>

package pipeline

import (
	"github.com/usnistgov/ndn-dpdk/ndn"
)

// Pipeline represents the congestion and retransmission logic of packets
// expressed by one consumer
type Pipeline interface {
	// Send returns a channel on which the consumer can express Interest packets
	Send() chan<- ndn.Interest

	// Get returns a channel on which Data packets are sent back
	Get() <-chan *ndn.Data

	// Error returns a channel to raise errors that might occur during packet
	// handling over NDN such as: maximum number of timeouts has been exceed, maximum
	// number of congestion NACKs has been exceeded
	Error() <-chan error

	Close()
}

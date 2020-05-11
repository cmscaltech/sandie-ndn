package xrdndndpdkproducer

import (
	"ndn-dpdk/container/pktqueue"
	"ndn-dpdk/core/nnduration"
	"ndn-dpdk/iface"
)

// InitConfigProducer config
type InitConfigProducer struct {
	Face     iface.LocatorWrapper // Face locator for face creation
	Producer *ProducerSettings    // If not nil, create a producer on the face
}

// ProducerSettings  config
type ProducerSettings struct {
	FreshnessPeriod nnduration.Milliseconds // FreshnessPeriod value
	RxQueue         pktqueue.Config
}

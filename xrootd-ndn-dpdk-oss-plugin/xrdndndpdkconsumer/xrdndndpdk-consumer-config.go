package xrdndndpdkconsumer

import (
	"time"

	"github.com/usnistgov/ndn-dpdk/iface"
)

// InitConfigConsumer config
type InitConfigConsumer struct {
	Face     iface.LocatorWrapper // Face locator for face creation
	Consumer *ConsumerSettings    // If not nil, create a consumer on the face
	Files    []string             // List of files to be copied over NDN by this consumer applications
}

// ConsumerSettings config
type ConsumerSettings struct {
	MustBeFresh      bool          // Whether to set MustBeFresh
	InterestLifetime time.Duration // InterestLifetime value, zero means default
	RxQueue          iface.PktQueueConfig
}

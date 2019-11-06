package xrdndndpdkconsumer

import (
	"time"

	"ndn-dpdk/iface"
)

// Package initialization config.
type InitConfig struct {
	QueueCapacity int // input-consumer queue capacity, must be power of 2
}

// Per-face task config for consumer
type TaskConfig struct {
	Face     iface.LocatorWrapper // face locator for face creation
	Consumer *ConsumerSettings    // if not nil, create a consumer on the face
	Files    []string             // list of file paths to be copied over NDN
}

// Consumer config.
type ConsumerSettings struct {
	MustBeFresh      bool          // whether to set MustBeFresh
	InterestLifetime time.Duration // InterestLifetime value, zero means default
}

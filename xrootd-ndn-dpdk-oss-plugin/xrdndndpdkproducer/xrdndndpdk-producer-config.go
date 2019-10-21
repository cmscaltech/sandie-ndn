package xrdndndpdkproducer

import (
	"time"

	"ndn-dpdk/iface"
)

// Package initialization config.
type InitConfig struct {
	QueueCapacity int // input-producer queue capacity, must be power of 2
}

// Per-face task config for producer
type TaskConfig struct {
	Face     iface.LocatorWrapper // face locator for face creation
	Producer *ProducerSettings    // if not nil, create a producer on the face
}

// Producer config.
type ProducerSettings struct {
	FreshnessPeriod time.Duration // FreshnessPeriod value
}

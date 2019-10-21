package main

import (
	"ndn-dpdk/core/logger"
)

var (
	log           = logger.New("xrdndndpdkconsumer")
	makeLogFields = logger.MakeFields
	addressOf     = logger.AddressOf
)

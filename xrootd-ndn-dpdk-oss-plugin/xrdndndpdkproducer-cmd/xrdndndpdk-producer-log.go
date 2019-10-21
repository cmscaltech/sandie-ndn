package main

import (
	"ndn-dpdk/core/logger"
)

var (
	log           = logger.New("xrdndndpdkproducer")
	makeLogFields = logger.MakeFields
	addressOf     = logger.AddressOf
)

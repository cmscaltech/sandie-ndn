// Copyright (C) 2020 California Institute of Technology
// Author: Catalin Iordache <catalin.iordache@cern.ch>

package main

import (
	"flag"
	"github.com/usnistgov/ndn-dpdk/core/macaddr"
	"github.com/usnistgov/ndn-dpdk/ndn/l3"
	"os"
	"os/signal"
	"sandie-ndn/go/internal/app/consumer"
	"sandie-ndn/go/internal/pkg/namespace"
	"syscall"
)

var (
	gqluri = flag.String("gqlserver", "http://localhost:3030/", "GraphQL API of local forwarder")
	ifname = flag.String("i", "", "network interface name")
	rxq    = flag.Int("rxq", l3.DefaultTransportRxQueueSize, "RX queue size")
	txq    = flag.Int("txq", l3.DefaultTransportTxQueueSize, "TX queue size")
	local  macaddr.Flag
	remote macaddr.Flag

	input  = flag.String("input", "", "input filepath")
	output = flag.String("output", "", "output filepath")
)

func init() {
	flag.Var(&local, "local", "local MAC address")
	flag.Var(&remote, "remote", "remote MAC address")
}

func exit(app *consumer.App,
	code int,
) {
	if e := app.Close(); e != nil {
		log.WithError(e).Fatal("close error")
	}

	os.Exit(code)
}

// main
func main() {
	flag.Parse()
	if *input == "" {
		flag.Usage()
		os.Exit(namespace.EFailure)
	}

	var appConfig consumer.AppConfig
	appConfig.Local = local.HardwareAddr
	appConfig.Remote = remote.HardwareAddr
	appConfig.RxQueueSize = *rxq
	appConfig.TxQueueSize = *txq
	appConfig.Ifname = *ifname
	appConfig.Gqluri = *gqluri
	appConfig.Input = *input
	appConfig.Output = *output

	app, e := consumer.NewApp(appConfig)
	if e != nil {
		log.WithError(e).Error("init error")
		exit(app, namespace.EFailure)
	}

	signals := make(chan os.Signal)
	signal.Notify(signals, syscall.SIGINT)

	go func() {
		<-signals
		exit(app, namespace.EFailure)
	}()

	if e := app.Run(); e != nil {
		log.WithError(e).Error("run error")
		exit(app, namespace.EFailure)
	}
}

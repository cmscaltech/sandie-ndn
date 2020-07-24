package main

import (
	"flag"
	"os"

	"sandie-ndn/xrootd-ndn-dpdk-oss-plugin/xrdndndpdkproducer"

	"github.com/usnistgov/ndn-dpdk/core/yamlflag"
	"github.com/usnistgov/ndn-dpdk/dpdk/ealthread"
	"github.com/usnistgov/ndn-dpdk/dpdk/pktmbuf"
	"github.com/usnistgov/ndn-dpdk/iface/createface"
)

type initConfig struct {
	Mempool    pktmbuf.TemplateUpdates
	LCoreAlloc ealthread.AllocConfig
	Face       createface.Config
}

type parsedCommand struct {
	initCfg         initConfig
	initCfgProducer []xrdndndpdkproducer.InitConfigProducer
}

func parseCommand(args []string) (pc parsedCommand, e error) {
	flags := flag.NewFlagSet(os.Args[0], flag.ExitOnError)
	flags.Var(yamlflag.New(&pc.initCfg), "initcfg", "initialization config object")
	flags.Var(yamlflag.New(&pc.initCfgProducer), "initcfgproducer", "xrdndndpdkproducer config object")

	e = flags.Parse(args)
	return pc, e
}

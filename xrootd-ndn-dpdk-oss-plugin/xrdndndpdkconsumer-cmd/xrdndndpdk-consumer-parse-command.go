package main

import (
	"flag"
	"os"

	"sandie-ndn/xrootd-ndn-dpdk-oss-plugin/xrdndndpdkconsumer"

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
	initCfg initConfig
	tasks   []xrdndndpdkconsumer.TaskConfig
}

func parseCommand(args []string) (pc parsedCommand, e error) {
	flags := flag.NewFlagSet(os.Args[0], flag.ExitOnError)
	flags.Var(yamlflag.New(&pc.initCfg), "initcfg", "initialization config object")
	flags.Var(yamlflag.New(&pc.tasks), "tasks", "xrdndndpdkconsumer task description")

	e = flags.Parse(args)
	return pc, e
}

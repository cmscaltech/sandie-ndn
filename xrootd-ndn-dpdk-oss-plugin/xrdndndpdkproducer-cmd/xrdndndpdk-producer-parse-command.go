package main

import (
	"flag"
	"os"

	"ndn-dpdk/appinit"
	"sandie-ndn/xrootd-ndn-dpdk-oss-plugin/xrdndndpdkproducer"
)

type initConfig struct {
	appinit.InitConfig `yaml:",inline"`
}

type parsedCommand struct {
	initCfg         initConfig
	initCfgProducer []xrdndndpdkproducer.InitConfigProducer
}

func parseCommand(args []string) (pc parsedCommand, e error) {
	flags := flag.NewFlagSet(os.Args[0], flag.ExitOnError)
	appinit.DeclareInitConfigFlag(flags, &pc.initCfg)
	appinit.DeclareConfigFlag(flags, &pc.initCfgProducer, "initcfgproducer", "xrdndndpdkproducer config object")

	e = flags.Parse(args)
	return pc, e
}

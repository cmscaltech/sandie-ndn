package main

import (
	"flag"
	"os"
	"time"

	"ndn-dpdk/appinit"
	"sandie-ndn/xrootd-ndn-dpdk-oss-plugin/xrdndndpdkproducer"
)

type initConfig struct {
	appinit.InitConfig `yaml:",inline"`
	ProducerInitConfig xrdndndpdkproducer.InitConfig
}

type parsedCommand struct {
	initCfg         initConfig
	tasks           []xrdndndpdkproducer.TaskConfig
	counterInterval time.Duration
}

func parseCommand(args []string) (pc parsedCommand, e error) {
	pc.initCfg.ProducerInitConfig.QueueCapacity = 256

	flags := flag.NewFlagSet(os.Args[0], flag.ExitOnError)
	appinit.DeclareInitConfigFlag(flags, &pc.initCfg)
	appinit.DeclareConfigFlag(flags, &pc.tasks, "tasks", "xrdndndpdkproducer task description")

	e = flags.Parse(args)
	return pc, e
}

package main

import (
	"flag"
	"os"

	"ndn-dpdk/appinit"
	"sandie-ndn/xrootd-ndn-dpdk-oss-plugin/xrdndndpdkconsumer"
)

type initConfig struct {
	appinit.InitConfig `yaml:",inline"`
	ConsumerInitConfig xrdndndpdkconsumer.InitConfig
}

type parsedCommand struct {
	initCfg initConfig
	tasks   []xrdndndpdkconsumer.TaskConfig
}

func parseCommand(args []string) (pc parsedCommand, e error) {
	pc.initCfg.ConsumerInitConfig.QueueCapacity = 256

	flags := flag.NewFlagSet(os.Args[0], flag.ExitOnError)
	appinit.DeclareInitConfigFlag(flags, &pc.initCfg)
	appinit.DeclareConfigFlag(flags, &pc.tasks, "tasks", "xrdndndpdkconsumer task description")

	e = flags.Parse(args)
	return pc, e
}

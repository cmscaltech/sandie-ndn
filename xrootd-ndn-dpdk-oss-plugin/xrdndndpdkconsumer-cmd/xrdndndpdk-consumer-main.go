package main

import (
	"os"

	"sandie-ndn/xrootd-ndn-dpdk-oss-plugin/xrdndndpdkconsumer"

	"github.com/usnistgov/ndn-dpdk/dpdk/ealinit"
	"github.com/usnistgov/ndn-dpdk/dpdk/ealthread"
	"github.com/usnistgov/ndn-dpdk/mgmt"
	"github.com/usnistgov/ndn-dpdk/mgmt/facemgmt"
	"github.com/usnistgov/ndn-dpdk/mgmt/versionmgmt"
)

func main() {
	pc, e := parseCommand(ealinit.Init(os.Args)[1:])
	if e != nil {
		log.WithError(e).Fatal("command line error")
	}

	pc.initCfg.Mempool.Apply()
	ealthread.DefaultAllocator.Config = pc.initCfg.LCoreAlloc
	pc.initCfg.Face.Apply()

	app, e := xrdndndpdkconsumer.New(pc.tasks[0])
	if e != nil {
		log.WithError(e).Fatal("xrdndndpdkconsumer.NewApp error")
	}

	app.Launch()

	mgmt.Register(versionmgmt.VersionMgmt{})
	mgmt.Register(facemgmt.FaceMgmt{})
	mgmt.Start()

	e = app.Run()
	if e != nil {
		log.WithError(e).Fatal("xrdndndpdkconsumer.Run error")
	}
}

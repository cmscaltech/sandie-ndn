package main

import (
	"os"

	"ndn-dpdk/appinit"
	"ndn-dpdk/dpdk"
	"ndn-dpdk/mgmt/facemgmt"
	"ndn-dpdk/mgmt/versionmgmt"
	"sandie-ndn/xrootd-ndn-dpdk-oss-plugin/xrdndndpdkproducer"
)

func main() {
	pc, e := parseCommand(dpdk.MustInitEal(os.Args)[1:])
	if e != nil {
		log.WithError(e).Fatal("command line error")
	}

	pc.initCfg.InitConfig.Apply()

	app, e := xrdndndpdkproducer.New(pc.tasks[0], pc.initCfg.ProducerInitConfig)
	if e != nil {
		log.WithError(e).Fatal("xrdndndpdkproducer.NewApp error")
	}

	app.Launch()

	appinit.RegisterMgmt(versionmgmt.VersionMgmt{})
	appinit.RegisterMgmt(facemgmt.FaceMgmt{})
	appinit.StartMgmt()

	select {}
}

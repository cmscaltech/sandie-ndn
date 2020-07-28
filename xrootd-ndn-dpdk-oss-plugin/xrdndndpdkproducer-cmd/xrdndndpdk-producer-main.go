package main

import (
	"os"
	"os/signal"
	"syscall"

	"sandie-ndn/xrootd-ndn-dpdk-oss-plugin/xrdndndpdkproducer"

	"github.com/usnistgov/ndn-dpdk/dpdk/ealinit"
	"github.com/usnistgov/ndn-dpdk/dpdk/ealthread"
	"github.com/usnistgov/ndn-dpdk/mgmt"
	"github.com/usnistgov/ndn-dpdk/mgmt/facemgmt"
	"github.com/usnistgov/ndn-dpdk/mgmt/versionmgmt"
)

func exit(app *xrdndndpdkproducer.App) {
	log.Info("Stopping XRootD NDN-DPDK Producer application")

	if e := app.Close(); e != nil {
		log.WithError(e).Fatal("On stopping XRootD NDN-DPDK Producer application")
	}
}

func main() {
	pc, e := parseCommand(ealinit.Init(os.Args)[1:])
	if e != nil {
		log.WithError(e).Fatal("Command line error")
	}

	pc.initCfg.Mempool.Apply()
	ealthread.DefaultAllocator.Config = pc.initCfg.LCoreAlloc

	app, e := xrdndndpdkproducer.New(pc.initCfgProducer[0])
	if e != nil {
		log.WithError(e).Fatal("Error on New App")
	}

	signalChan := make(chan os.Signal)
	signal.Notify(signalChan, os.Interrupt, syscall.SIGTERM)
	go func() {
		<-signalChan
		exit(app)
		os.Exit(1)
	}()

	log.Info("Starting XRootD NDN-DPDK Producer application")
	e = app.Launch()
	if e != nil {
		log.WithError(e).Fatal("App lauch error")
	}

	mgmt.Register(versionmgmt.VersionMgmt{})
	mgmt.Register(facemgmt.FaceMgmt{})
	mgmt.Start()

	select {}
}

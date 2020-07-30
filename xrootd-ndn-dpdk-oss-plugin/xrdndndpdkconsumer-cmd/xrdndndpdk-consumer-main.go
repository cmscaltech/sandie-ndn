package main

import (
	"os"
	"os/signal"
	"syscall"

	"sandie-ndn/xrootd-ndn-dpdk-oss-plugin/xrdndndpdkconsumer"

	"github.com/usnistgov/ndn-dpdk/dpdk/ealinit"
	"github.com/usnistgov/ndn-dpdk/dpdk/ealthread"
	"github.com/usnistgov/ndn-dpdk/mgmt"
	"github.com/usnistgov/ndn-dpdk/mgmt/facemgmt"
)

func exit(app *xrdndndpdkconsumer.App) {
	log.Info("Stopping XRootD NDN-DPDK Consumer application")

	if e := app.Close(); e != nil {
		log.WithError(e).Fatal("On stopping XRootD NDN-DPDK Consumer application")
	}
}

func main() {
	pc, e := parseCommand(ealinit.Init(os.Args)[1:])
	if e != nil {
		log.WithError(e).Fatal("Command line error")
	}

	pc.initCfg.Mempool.Apply()
	ealthread.DefaultAllocator.Config = pc.initCfg.LCoreAlloc

	app, e := xrdndndpdkconsumer.New(pc.initCfgConsumer[0])
	if e != nil {
		log.WithError(e).Fatal("New App error")
	}

	signalChan := make(chan os.Signal)
	signal.Notify(signalChan, os.Interrupt, syscall.SIGTERM)
	go func() {
		<-signalChan
		exit(app)
		os.Exit(1)
	}()

	log.Info("Starting XRootD NDN-DPDK Consumer application")
	if e = app.Launch(); e != nil {
		log.WithError(e).Fatal("App launch error")
	}

	mgmt.Register(facemgmt.FaceMgmt{})
	mgmt.Start()

	if e = app.Run(); e != nil {
		log.WithError(e).Fatal("App run error")
	}
}

package main

import (
	"os"
	"os/signal"
	"syscall"

	"ndn-dpdk/appinit"
	"ndn-dpdk/dpdk"
	"ndn-dpdk/mgmt/facemgmt"
	"ndn-dpdk/mgmt/versionmgmt"
	"sandie-ndn/xrootd-ndn-dpdk-oss-plugin/xrdndndpdkproducer"
)

func exit(app *xrdndndpdkproducer.App) {
	log.Info("Stopping XRootD NDN-DPDK Producer application")

	if e := app.Stop(); e != nil {
		log.WithError(e).Fatal("On stopping XRootD NDN-DPDK Producer application")
	}

	if e := app.Close(); e != nil {
		log.WithError(e).Fatal("On stopping XRootD NDN-DPDK Producer application")
	}
}

func main() {
	pc, e := parseCommand(dpdk.MustInitEal(os.Args)[1:])
	if e != nil {
		log.WithError(e).Fatal("Command line error")
	}

	pc.initCfg.InitConfig.Apply()

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

	appinit.RegisterMgmt(versionmgmt.VersionMgmt{})
	appinit.RegisterMgmt(facemgmt.FaceMgmt{})
	appinit.StartMgmt()

	select {}
}

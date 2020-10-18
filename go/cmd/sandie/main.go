// Copyright (C) 2020 California Institute of Technology
// Author: Catalin Iordache <catalin.iordache@cern.ch>

package main

import (
	"github.com/urfave/cli/v2"
	"github.com/usnistgov/ndn-dpdk/mk/version"
	"github.com/usnistgov/ndn-dpdk/ndn/l3"
	"github.com/usnistgov/ndn-dpdk/ndn/memiftransport"
	"github.com/usnistgov/ndn-dpdk/ndn/mgmt"
	"github.com/usnistgov/ndn-dpdk/ndn/mgmt/gqlmgmt"
	"os"
	"os/signal"
	"sort"
	"syscall"
)

var (
	interrupt = make(chan os.Signal, 1)
	client    *gqlmgmt.Client
	face      mgmt.Face
	fwFace    l3.FwFace
)

func openUplink(*cli.Context) (e error) {

	if face, e = client.OpenMemif(memiftransport.Locator {
		Dataroom: 9000,
	}); e != nil {
		return e
	}

	fw := l3.GetDefaultForwarder()
	if fwFace, e = fw.AddFace(face.Face()); e != nil {
		return e
	}

	fw.AddReadvertiseDestination(face)
	log.Info("uplink opened")
	return nil
}

var app = &cli.App{
	Version: version.Get().String(),
	Usage:   "NDNgo SANDIE.",
	Flags: []cli.Flag{
		&cli.StringFlag{
			Name:    "gqlserver",
			Value:   "http://127.0.0.1:3030/",
			Usage:   "GraphQL `endpoint` of NDN-DPDK daemon.",
			EnvVars: []string{"GQLSERVER"},
		},
	},
	Before: func(c *cli.Context) (e error) {
		signal.Notify(interrupt, syscall.SIGINT)
		client, e = gqlmgmt.New(c.String("gqlserver"))
		return e
	},
	After: func(c *cli.Context) (e error) {
		if face != nil {
			if e = face.Close(); e != nil {
				log.WithError(e).Warn("closing uplink")
			} else {
				log.Info("uplink closed")
			}
		}
		return client.Close()
	},
}

func defineCommand(command *cli.Command) {
	app.Commands = append(app.Commands, command)
}

func main() {
	e := app.Run(os.Args)

	sort.Sort(cli.CommandsByName(app.Commands))
	if e != nil {
		log.WithError(e).Fatal("run error")
	}
}

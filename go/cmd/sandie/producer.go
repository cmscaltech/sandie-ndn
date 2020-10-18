// Copyright (C) 2020 California Institute of Technology
// Author: Catalin Iordache <catalin.iordache@cern.ch>

package main

import (
	"github.com/urfave/cli/v2"

	sandie "github.com/cmscaltech/sandie-ndn/go/internal/pkg/producer"
)

var (
	producer *sandie.Producer
)

func init() {
	defineCommand(&cli.Command{
		Name:  "producer",
		Usage: "NDNgo SANDIE producer.",
		Flags: []cli.Flag{
		},
		Before: openUplink,
		Action: func(c *cli.Context) (e error) {
			producer, e = sandie.NewProducer(c)

			if e != nil {
				return e
			}
			<-interrupt
			return nil
		},
		After: func(context *cli.Context) (e error) {
			if e = producer.Close(); e != nil {
				log.WithError(e).Fatal("closing producer")
			}
			return e
		},
	})
}

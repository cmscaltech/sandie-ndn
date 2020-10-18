// Copyright (C) 2020 California Institute of Technology
// Author: Catalin Iordache <catalin.iordache@cern.ch>

package producer

import (
	"context"
	"github.com/cmscaltech/sandie-ndn/go/internal/pkg/filesystem"
	"github.com/cmscaltech/sandie-ndn/go/internal/pkg/namespace"
	"github.com/cmscaltech/sandie-ndn/go/internal/pkg/utils"
	"github.com/urfave/cli/v2"
	"github.com/usnistgov/ndn-dpdk/ndn"
	"github.com/usnistgov/ndn-dpdk/ndn/endpoint"
	"os"
)

type Producer struct {
	context *cli.Context
	endpoints []interface{}
}

func NewProducer(context *cli.Context) (producer *Producer, e error) {
	log.Debug("get new producer")

	producer = new(Producer)
	producer.context = context

	if e = producer.startEndpointProducer(namespace.NamePrefixUriFileinfo); e != nil {
		return nil, e
	}

	if e = producer.startEndpointProducer(namespace.NamePrefixUriFileRead); e != nil {
		return nil, e
	}

	return producer, nil
}

func (p *Producer) startEndpointProducer(prefix string) (e error) {
	log.Info("start endpoint producer for prefix: ", prefix)

	endpointProducer, e := endpoint.Produce(context.Background(), endpoint.ProducerOptions {
		Prefix: ndn.ParseName(prefix),
		Handler: p.getEndpointHandler(prefix),
	})

	if e != nil {
		return e
	}

	p.endpoints = append(p.endpoints, endpointProducer)
	return nil
}

func (p *Producer) getEndpointHandler(prefix string) endpoint.ProducerHandler {
	switch prefix {
	case namespace.NamePrefixUriFileinfo:
		return func(ctx context.Context, interest ndn.Interest) (ndn.Data, error) {
			log.Info("received fileinfo interest: ", interest.Name.String())

			file, err := os.Open(
				utils.GetFilePath(interest.Name.Slice(namespace.NamePrefixUriFileinfoComponents)))
			if err != nil {
				log.Fatal(err)
			}

			fileinfo, _ := file.Stat()
			log.Info("File size: ", fileinfo.Size())

			payload, e := filesystem.MarshalFileInfo(filesystem.FileInfo{
				Size: fileinfo.Size(),
			})

			if e != nil {
				panic(e) // TODO, Create NACK packet signaling error for file
			}

			return ndn.MakeData(interest, payload), nil
		}
	case namespace.NamePrefixUriFileRead:
		log.Info("Return Endpoint Handler for Read")

		return func(ctx context.Context, interest ndn.Interest) (ndn.Data, error) {
			log.Debug("received read interest: ", interest.Name.String())

			payload := make([]byte, 6144)
			return ndn.MakeData(interest, payload), nil
		}
	}

	log.Fatal("no available handler for prefix: ", prefix)
	return nil
}

func (p *Producer) Close() (e error) {
	log.Debug("closing producer")

	for _, v := range p.endpoints {
		e = v.(endpoint.Producer).Close()

		if e != nil {
			return e
		}
	}

	return nil
}

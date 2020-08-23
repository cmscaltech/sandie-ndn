// Copyright (C) 2020 California Institute of Technology
// Author: Catalin Iordache <catalin.iordache@cern.ch>

package pipeline

import (
	"context"
	"fmt"
	"github.com/usnistgov/ndn-dpdk/ndn"
	"github.com/usnistgov/ndn-dpdk/ndn/an"
	"github.com/usnistgov/ndn-dpdk/ndn/l3"
	"time"

	"sandie-ndn/go/internal/pkg/namespace"
)

func NewFixed(face l3.Face, size int) (Pipeline, error) {
	if size > 1024 {
		return nil, fmt.Errorf("fixed window size: %d for pipeline too large", size)
	}

	p := &fixed{
		face: face,

		onSend:  make(chan ndn.Name, 1024),
		onData:  make(chan *ndn.Data, 1024),
		onError: make(chan error),
		window:  make(chan bool, size), // Fixed window size
	}

	p.ctx, p.cancel = context.WithCancel(context.Background())

	go p.sendLoop()
	go p.receiveLoop()
	return p, nil
}

type fixed struct {
	face l3.Face

	onSend  chan ndn.Name
	onData  chan *ndn.Data
	onError chan error
	window  chan bool

	ctx    context.Context
	cancel context.CancelFunc
}

func (pipe *fixed) Close() {
	log.Debug("Closing pipeline")
	pipe.cancel() // Close all background workers
	close(pipe.window)
}

func (pipe *fixed) Face() l3.Face {
	return pipe.face
}

func (pipe *fixed) Send() chan<- ndn.Name {
	return pipe.onSend
}

func (pipe *fixed) OnData() <-chan *ndn.Data {
	return pipe.onData
}

func (pipe *fixed) OnFailure() <-chan error {
	return pipe.onError
}

func (pipe *fixed) sendLoop() {
	for name := range pipe.onSend {
		pipe.window <- true // Wait for available spot in fixed window
		pipe.face.Tx() <- ndn.MakeInterest(name, ndn.NewNonce(), namespace.DefaultInterestLifetime)
	}

	close(pipe.onSend)
}

func (pipe *fixed) receiveLoop() {
	for packet := range pipe.Face().Rx() {
		<-pipe.window // Free one spot in fixed window

		switch {
		case packet.Data != nil:
			log.Debug("Received Data packet: ", packet.Data.Name)
			pipe.onData <- packet.Data

		case packet.Nack != nil && packet.Nack.Reason == an.NackCongestion:
			log.Warn(
				"Received ",
				an.NackReasonString(packet.Nack.Reason),
				"for packet: ",
				packet.Nack.Name(),
			)
			// Backoff, then send a refresh Interest again
			go func(ctx context.Context, name ndn.Name) {
				for {
					select {
					case <-ctx.Done():
						return
					case <-time.After(2 * time.Second):
						pipe.face.Tx() <- ndn.MakeInterest(name, ndn.NewNonce(), namespace.DefaultInterestLifetime)
					}
				}
			}(pipe.ctx, packet.Nack.Name())

		case packet.Nack != nil && packet.Nack.Reason == an.NackDuplicate:
			log.Warn(
				"Received ",
				an.NackReasonString(packet.Nack.Reason),
				"for packet: ",
				packet.Nack.Name(),
			)
			pipe.face.Tx() <- ndn.MakeInterest(packet.Nack.Name(), ndn.NewNonce(), namespace.DefaultInterestLifetime)

		case packet.Nack != nil:
			pipe.onError <- fmt.Errorf(
				"unsupported NACK reason %s for packet: %s",
				an.NackReasonString(packet.Nack.Reason),
				packet.Nack.Name().String(),
			)

		default:
			pipe.onError <- fmt.Errorf("unsupported packet type")
		}
	}

	close(pipe.onData)
	close(pipe.onError)
}

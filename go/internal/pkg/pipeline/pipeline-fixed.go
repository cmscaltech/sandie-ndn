// Copyright (C) 2020 California Institute of Technology
// Author: Catalin Iordache <catalin.iordache@cern.ch>

package pipeline

import (
	"context"
	"fmt"
	"github.com/usnistgov/ndn-dpdk/ndn"
	"github.com/usnistgov/ndn-dpdk/ndn/an"
	"github.com/usnistgov/ndn-dpdk/ndn/l3"
	"sandie-ndn/go/internal/pkg/namespace"
	"sync"
	"time"
)

func NewFixed(face l3.Face) (Pipeline, error) {
	p := &fixed{
		face:              face,
		onSend:            make(chan ndn.Interest, 512),
		onData:            make(chan *ndn.Data, 1024),
		onError:           make(chan error),
		windowFlowControl: make(chan bool, 64), // Fixed window size
	}
	p.ctx, p.cancel = context.WithCancel(context.Background())

	go p.sendLoop()
	go p.receiveLoop()
	return p, nil
}

type fixed struct {
	face l3.Face

	onSend  chan ndn.Interest
	onData  chan *ndn.Data
	onError chan error

	windowFlowControl chan bool
	windowEntries     sync.Map

	ctx    context.Context
	cancel context.CancelFunc
}

func (pipe *fixed) Close() {
	log.Debug("Closing fixed window size pipeline")

	pipe.cancel() // close all workers
	close(pipe.onData)
	close(pipe.onError)
	close(pipe.windowFlowControl)
}

func (pipe *fixed) Send() chan<- ndn.Interest {
	return pipe.onSend
}

func (pipe *fixed) OnData() <-chan *ndn.Data {
	return pipe.onData
}

func (pipe *fixed) OnFailure() <-chan error {
	return pipe.onError
}

func (pipe *fixed) sendLoop() {
	for interest := range pipe.onSend {
		pipe.windowFlowControl <- true // wait for window to slide
		pipe.windowEntries.Store(interest.Name.String(), Entry{interest: interest})
		pipe.face.Tx() <- interest
	}

	log.Debug("Stopping fixed window size pipeline send loop")
	close(pipe.onSend)
}

func (pipe *fixed) receiveLoop() {
	for {
		select {
		case <-pipe.ctx.Done():
			log.Debug("Stopping fixed window size pipeline receive loop")
			return

		case packet := <-pipe.face.Rx():
			switch {
			case packet.Data != nil:
				<-pipe.windowFlowControl // slide window
				pipe.windowEntries.Delete(packet.Data.Name.String())
				pipe.onData <- packet.Data

			case packet.Nack != nil && packet.Nack.Reason == an.NackCongestion:
				log.Warn(
					"Received ",
					an.NackReasonString(packet.Nack.Reason),
					" for packet: ",
					packet.Nack.Name(),
				)

				if entry, ok := pipe.windowEntries.Load(packet.Nack.Name().String()); !ok {
					pipe.onError <- fmt.Errorf("received packet is not present in the window")
				} else {
					ent := entry.(Entry)
					ent.nNackCongestion++

					if ent.nNackCongestion > MaxNackCongestion {
						pipe.onError <- fmt.Errorf("reached the maximum number of Nack retries: %d", MaxNackCongestion)
						return
					}

					ent.interest.Nonce = ndn.NewNonce()
					pipe.windowEntries.Store(packet.Nack.Name().String(), ent)

					go func(interest ndn.Interest) { //Backoff and resend Interest packet
						select {
						case <-pipe.ctx.Done():
							return
						case <-time.After(MaxCongestionBackoffTime):
							pipe.face.Tx() <- interest
						}
					}(ent.interest)
				}

			case packet.Nack != nil && packet.Nack.Reason == an.NackDuplicate:
				log.Warn(
					"Received ",
					an.NackReasonString(packet.Nack.Reason),
					" for packet: ",
					packet.Nack.Name(),
				)

				if entry, ok := pipe.windowEntries.Load(packet.Nack.Name().String()); !ok {
					pipe.onError <- fmt.Errorf("received packet is not present in the window")
				} else {
					ent := entry.(Entry)
					ent.interest.Nonce = ndn.NewNonce()
					pipe.windowEntries.Store(packet.Nack.Name().String(), ent)
					pipe.face.Tx() <- ent.interest
				}

			default:
				pipe.onError <- fmt.Errorf("unsupported packet type")
			}

		case <-time.After(namespace.DefaultInterestLifetime):
			log.Warn("Received timeout")

			pipe.windowEntries.Range(func(key, value interface{}) bool {
				ent := value.(Entry)
				ent.nTimeouts++

				if ent.nTimeouts > MaxTimeoutRetries {
					pipe.onError <- fmt.Errorf("reached the maximum number of timeout retries: %d", MaxTimeoutRetries)
					return false
				}

				ent.interest.Nonce = ndn.NewNonce()
				pipe.windowEntries.Store(key, ent)
				pipe.face.Tx() <- ent.interest
				return true
			})
		}
	}
}

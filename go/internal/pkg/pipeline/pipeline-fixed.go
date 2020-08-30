// Copyright (C) 2020 California Institute of Technology
// Author: Catalin Iordache <catalin.iordache@cern.ch>

package pipeline

import (
	"github.com/usnistgov/ndn-dpdk/ndn"
	"github.com/usnistgov/ndn-dpdk/ndn/l3"
)

func NewFixed(face l3.Face) (Pipeline, error) {
	p := &fixed{
		interestChannel: make(chan ndn.Interest, 512),
		dataChannel:     make(chan *ndn.Data, 1024),
		errorChannel:    make(chan error),
		window:          make(chan bool, 64), // Fixed window size
	}
	p.fetcher = NewFetcher(face, p.errorChannel)

	go p.sendLoop()
	go p.receiveLoop()
	return p, nil
}

type fixed struct {
	fetcher Fetcher
	window  chan bool

	interestChannel chan ndn.Interest
	dataChannel     chan *ndn.Data
	errorChannel    chan error
}

func (pipe *fixed) Close() {
	log.Debug("Closing fixed window size pipeline")
	pipe.fetcher.Stop()
}

func (pipe *fixed) Send() chan<- ndn.Interest {
	return pipe.interestChannel
}

func (pipe *fixed) Get() <-chan *ndn.Data {
	return pipe.dataChannel
}

func (pipe *fixed) Error() <-chan error {
	return pipe.errorChannel
}

func (pipe *fixed) sendLoop() {
	for interest := range pipe.interestChannel {
		pipe.window <- true // wait for window to slide
		pipe.fetcher.In(interest)
	}

	close(pipe.window)
	close(pipe.interestChannel)
}

func (pipe *fixed) receiveLoop() {
	for data := range pipe.fetcher.Out() {
		<-pipe.window            // slide window
		pipe.dataChannel <- data // send data back to the consumer
	}

	close(pipe.dataChannel)
	close(pipe.errorChannel)
}

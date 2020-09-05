// Copyright (C) 2020 California Institute of Technology
// Author: Catalin Iordache <catalin.iordache@cern.ch>

package pipeline

import (
	"context"
	"fmt"
	"github.com/usnistgov/ndn-dpdk/ndn"
	"github.com/usnistgov/ndn-dpdk/ndn/an"
	"github.com/usnistgov/ndn-dpdk/ndn/l3"
	"math"
	"sync"
	"time"
)

var (
	MaxNackCongestion        = 4
	MaxCongestionBackoffTime = time.Second * 8
	MaxTimeoutRetries        = 4
)

type Fetcher interface {
	In(interest ndn.Interest)
	Out() <-chan *ndn.Data

	Stop()
}

// Entry represents one expressed Interest next to congestion and retransmission
// parameters
type Entry struct {
	interest ndn.Interest
	lifetime time.Time

	nNackCongestion int
	nNackDuplicate  int
	nTimeouts       int
}

func NewFetcher(face l3.Face,
	onError chan<- error,
) Fetcher {
	f := &fetcher{
		face:    face,
		timeout: make(chan bool),
		ch:      make(chan *ndn.Data),
	}

	f.ctx, f.cancel = context.WithCancel(context.Background())

	go func() {
		for packet := range face.Rx() {
			switch {
			case packet.Data != nil:
				f.handleData(packet.Data, onError)
			case packet.Nack != nil:
				f.handleNack(packet.Nack, onError)
			default:
				onError <- fmt.Errorf("unsupported packet type")
			}
		}

		close(f.ch)
		close(f.timeout)
	}()

	go func() {
		for {
			select {
			case <-f.ctx.Done():
				return
			case <-f.timeout: // reset timeout timer
			case <-time.After(time.Second * 2):
				f.handleTimeout(onError)
			}
		}
	}()

	return f
}

type fetcher struct {
	face    l3.Face
	entries sync.Map

	ctx    context.Context
	cancel context.CancelFunc

	timeout chan bool
	ch      chan *ndn.Data
}

func (f *fetcher) In(interest ndn.Interest) {
	interest.Nonce = ndn.NewNonce()

	f.entries.Store(
		interest.Name.String(),
		Entry{
			interest: interest,
			lifetime: time.Now(),
		},
	)
	f.face.Tx() <- interest
}

// resend used for resending Interest packets
func (f *fetcher) resend(entry Entry) {
	entry.interest.Nonce = ndn.NewNonce()
	entry.lifetime = time.Now()

	f.entries.Store(entry.interest.Name.String(), entry)
	f.face.Tx() <- entry.interest
}

func (f *fetcher) Out() <-chan *ndn.Data {
	return f.ch
}

func (f *fetcher) Stop() {
	f.cancel()
}

func (f *fetcher) handleData(data *ndn.Data,
	onError chan<- error,
) {
	f.entries.Delete(data.Name.String())
	f.timeout <- false
	f.ch <- data
}

func (f *fetcher) handleNack(nack *ndn.Nack,
	onError chan<- error,
) {
	log.Warn("Received ", an.NackReasonString(nack.Reason), " for packet: ", nack.Name())

	switch nack.Reason {
	case an.NackCongestion:
		if entry, ok := f.entries.Load(nack.Name().String()); !ok {
			onError <- fmt.Errorf("received packet was not previously expressed")
		} else {
			ent := entry.(Entry)
			ent.nNackCongestion++

			if ent.nNackCongestion > MaxNackCongestion {
				onError <- fmt.Errorf(
					"reached the maximum number of Nack retries: %d",
					MaxNackCongestion,
				)
				return
			}

			backoff := time.Second * time.Duration(math.Pow(2, float64(ent.nNackCongestion)))
			if backoff > MaxCongestionBackoffTime {
				backoff = MaxCongestionBackoffTime
			}

			go func() { // backoff and resend Interest
				select {
				case <-f.ctx.Done():
					return
				case <-time.After(backoff):
					f.resend(ent)
				}
			}()
		}

	case an.NackDuplicate:
		if entry, ok := f.entries.Load(nack.Name().String()); !ok {
			onError <- fmt.Errorf("received packet is not present in the window")
		} else {
			f.resend(entry.(Entry))
		}

	default:
		onError <- fmt.Errorf("unsupported: %s packet type", an.NackReasonString(nack.Reason))
	}
}

func (f *fetcher) handleTimeout(onError chan<- error) {
	f.entries.Range(
		func(key, value interface{}) bool {
			ent := value.(Entry)
			if time.Now().Sub(ent.lifetime) < ent.interest.Lifetime { // Interest has not expired
				return true
			}

			if ent.nTimeouts++; ent.nTimeouts > MaxTimeoutRetries {
				onError <- fmt.Errorf(
					"reached the maximum number of timeout retries: %d",
					MaxTimeoutRetries,
				)
				return false
			}

			log.Warn("Timeout for Interest: ", ent.interest.Name)
			f.resend(ent)
			return true
		},
	)
}

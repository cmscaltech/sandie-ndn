// Copyright (C) 2020 California Institute of Technology
// Author: Catalin Iordache <catalin.iordache@cern.ch>

package consumer

import (
	"fmt"
	"github.com/usnistgov/ndn-dpdk/ndn"
	"github.com/usnistgov/ndn-dpdk/ndn/an"
	"github.com/usnistgov/ndn-dpdk/ndn/l3"
	"github.com/usnistgov/ndn-dpdk/ndn/memiftransport"
	"github.com/usnistgov/ndn-dpdk/ndn/mgmt/gqlmgmt"
	"github.com/usnistgov/ndn-dpdk/ndn/packettransport/afpacket"
	"github.com/usnistgov/ndn-dpdk/ndn/tlv"
	"sandie-ndn/go/internal/pkg/namespace"
	"sandie-ndn/go/internal/pkg/utils"
	"time"
)

// Consumer
type Consumer struct {
	face      l3.Face
	cleanup func() error
}

type FileInfo struct {
	Size uint64
	// TODO: Add more fields after implementing Pure-go producer (support Content encoding)
	// Mode uint32
}

var (
	dataCh = make(chan *ndn.Data)
	errorCh = make(chan error)
)

func createFaceLocal(gqluri string) (l3.Face, func() error, error) {
	c, e := gqlmgmt.New(gqluri)
	if e != nil {
		return nil, nil, e
	}

	var loc = memiftransport.Locator{Dataroom: 9000}

	f, e := c.OpenMemif(loc)
	if e != nil {
		return nil, nil, e
	}

	log.Info("Opening memif face on local forwarder ", f.ID())
	time.Sleep(1000 * time.Millisecond)

	return f.Face(), func() error {
		time.Sleep(1000 * time.Millisecond) // allow time to close face
		return c.Close()
	}, nil
}

func createFaceNet(settings Settings) (l3.Face, func() error, error) {
	tr, e := afpacket.New(settings.Ifname, settings.Config)
	if e != nil {
		return nil, nil, e
	}

	face, e := l3.NewFace(tr)
	if e != nil {
		return nil, nil, e
	}

	log.Info("Opening AF_PACKET face on network interface ", settings.Ifname)
	return face, func() error { return nil }, nil
}

// NewConsumer
func NewConsumer(settings Settings) (consumer *Consumer, e error) {
	consumer = new(Consumer)
	log.Debug("Get new Consumer")

	if settings.Ifname == "" {
		consumer.face, consumer.cleanup, e = createFaceLocal(settings.Gqluri)
	} else {
		consumer.face, consumer.cleanup, e = createFaceNet(settings)
	}

	if e != nil {
		return nil, e
	}

	if consumer.face == nil {
		return nil, fmt.Errorf("face is nil")
	}

	// Launch goroutine to process all incoming packets on Rx channel
	go consumer.OnPacket()
	return consumer, nil
}

// Close
func (consumer *Consumer) Close() error {
	log.Debug("Closing consumer")
	// Closing this channel causes the face to close
	// Rx channel will also be closed, likewise OnPacket goroutine
	close(consumer.face.Tx())

	if e := consumer.cleanup(); e != nil {
		return e
	}

	close(dataCh)
	close(errorCh)
	return nil
}

// OnPacket Process all packets on Rx channel
func (consumer *Consumer) OnPacket() {
	for packet := range consumer.face.Rx() {
		if packet.Data != nil {
			if packet.Data.ContentType != an.ContentNack {
				dataCh <- packet.Data
				continue
			}

			if contentV, e := utils.ReadNNI(packet.Data.Content); e != nil {
				errorCh <- e
			} else {
				errorCh <- fmt.Errorf(
					"application-level nack for packet: %s with errcode: %d",
					packet.Data.Name.String(),
					contentV,
				)
			}
		} else if packet.Nack != nil {
			switch packet.Nack.Reason {
			case an.NackCongestion: // TODO
			case an.NackDuplicate: // TODO
			case an.NackNoRoute:
			default:
				errorCh <- fmt.Errorf(
					"nack reason %s for packet: %s",
					an.NackReasonString(packet.Nack.Reason),
					packet.Nack.Name().String(),
				)
			}
		} else {
			errorCh <- fmt.Errorf("unsupported packet type")
		}
	}
}

func (consumer *Consumer) Stat(filepath string) (fi FileInfo, e error) {
	log.Info("Get fileinfo for: ", filepath)

	in := ndn.ParseName(namespace.NamePrefixUriFileinfo + filepath)
	consumer.face.Tx() <- ndn.MakeInterest(in, ndn.NewNonce(), namespace.DefaultInterestLifetime)

	select {
	case data := <-dataCh:
		{
			log.Debug("Received Data for fileinfo Interest")
			if contentV, e := utils.ReadNNI(data.Content); e == nil {
				fi.Size = uint64(contentV)
			}
		}
	case e = <-errorCh:
		return fi, nil
	}

	return fi, e
}

func (consumer *Consumer) ReadAt(b []byte, off int64, filepath string) (n int, e error) {
	log.Debug("Reading ", len(b), " bytes @", off, " from: ", filepath)

	var npkts = 0

	// Always request the same packets no matter the offset in file.
	// Round down to the nearest multiple of MaxPayloadSize.
	// Packet Name format: Prefix + filepath + ByteOffset
	var byteOffsetV = (off / namespace.MaxPayloadSize) * namespace.MaxPayloadSize
	for byteOffsetV < off+int64(len(b)) {
		byteOffsetNameComponent := tlv.MakeElementNNI(an.TtByteOffsetNameComponent, byteOffsetV)

		name := ndn.ParseName(namespace.NamePrefixUriFileRead + filepath)
		name = append(
			name,
			ndn.NameComponent{Element: byteOffsetNameComponent},
		)

		if consumer.face.State() != l3.TransportUp {
			return 0, nil
		}
		consumer.face.Tx() <- ndn.MakeInterest(name, ndn.NewNonce(), namespace.DefaultInterestLifetime)

		byteOffsetV += namespace.MaxPayloadSize
		npkts++
	}

	for i := 0; i < npkts; i++ {
		select {
		case data := <-dataCh:
			{
				if len(data.Content) == 0 {
					continue
				}

				// Read ByteOffsetNameComponent V
				byteOffset, e := utils.ReadNNI(data.Name.Get(-1).Value)
				if e != nil {
					return 0, e
				}

				if int64(byteOffset) < off { // Trim first segment
					n += copy(b, data.Content[off-int64(byteOffset):])
				} else {
					n += copy(b[int64(byteOffset)-off:], data.Content)
				}
			}
		case e = <-errorCh:
				return 0, e
		}
	}

	return n, nil
}

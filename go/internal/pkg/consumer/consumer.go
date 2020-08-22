// Copyright (C) 2020 California Institute of Technology
// Author: Catalin Iordache <catalin.iordache@cern.ch>

package consumer

import (
	"fmt"
	"github.com/usnistgov/ndn-dpdk/ndn"
	"github.com/usnistgov/ndn-dpdk/ndn/an"
	"github.com/usnistgov/ndn-dpdk/ndn/l3"
	"github.com/usnistgov/ndn-dpdk/ndn/mgmt/gqlmgmt"
	"github.com/usnistgov/ndn-dpdk/ndn/tlv"
	"sandie-ndn/go/internal/pkg/namespace"
	"sandie-ndn/go/internal/pkg/utils"
	"time"
)

// Consumer
type Consumer struct {
	face       l3.Face
	mgmtClient *gqlmgmt.Client
}

type FileInfo struct {
	Size uint64
	// TODO: Add more fields after implementing Pure-go producer (support Content encoding)
	// Mode uint32
}

var (
	channelData  = make(chan *ndn.Data)
	channelError = make(chan error)
)

// NewConsumer
func NewConsumer(config Config) (consumer *Consumer, e error) {
	consumer = new(Consumer)
	log.Debug("Get new consumer")

	if config.Ifname == "" {
		consumer.face, consumer.mgmtClient, e = utils.CreateFaceLocal(config.Gqluri)
	} else {
		consumer.face, e = utils.CreateFaceNet(config.Ifname, config.Config)
	}

	if e != nil {
		return nil, e
	}

	if consumer.face == nil {
		return nil, fmt.Errorf("face is nil")
	}

	// Launch goroutine to process all incoming packets on Rx channel
	go consumer.onPacket()
	return consumer, nil
}

// Close consumer
// Closing face.Tx() will close RX(), thus onPacket goroutine
func (consumer *Consumer) Close() {
	log.Debug("Closing consumer")
	close(consumer.face.Tx())

	if consumer.mgmtClient != nil {
		time.Sleep(100 * time.Millisecond) // allow time to close face
		_ = consumer.mgmtClient.Close()
	}

	close(channelData)
	close(channelError)
}

// onPacket process all packets on Rx channel
func (consumer *Consumer) onPacket() {
	for packet := range consumer.face.Rx() { // Handle Data
		if packet.Data != nil {
			log.Debug("Data for packet: ", packet.Data.Name)

			if packet.Data.ContentType != an.ContentNack {
				channelData <- packet.Data
				continue
			}

			if contentV, e := utils.ReadNNI(packet.Data.Content); e != nil {
				channelError <- e
			} else {
				channelError <- fmt.Errorf(
					"application-level NACK for packet: %s with errcode: %d",
					packet.Data.Name.String(),
					contentV,
				)
			}
		} else if packet.Nack != nil { // Handle Nack
			log.Debug(
				"NACK with reason ",
				an.NackReasonString(packet.Nack.Reason),
				" for packet: ",
				packet.Nack.Name(),
			)

			switch packet.Nack.Reason {
			case an.NackCongestion:
				go func(name ndn.Name) {
					time.Sleep(2 * time.Second)
					consumer.expressInterest(name)
				}(packet.Nack.Name())
			case an.NackDuplicate:
				consumer.expressInterest(packet.Nack.Name())
			default:
				channelError <- fmt.Errorf(
					"NACK with reason %s for packet: %s",
					an.NackReasonString(packet.Nack.Reason),
					packet.Nack.Name().String(),
				)
			}
		} else {
			channelError <- fmt.Errorf("unsupported packet type")
		}
	}
}

// expressInterest Put an Interest packet on Tx face
func (consumer *Consumer) expressInterest(name ndn.Name) {
	consumer.face.Tx() <- ndn.MakeInterest(name, ndn.NewNonce(), namespace.DefaultInterestLifetime)
}

// Stat Express Interest type: FILEINFO, for getting Data with Content FileInfo struct for file
func (consumer *Consumer) Stat(filepath string) (fi FileInfo, e error) {
	log.Debug("Stat: ", filepath)
	consumer.expressInterest(ndn.ParseName(namespace.NamePrefixUriFileinfo + filepath))

	select {
	case data := <-channelData:
		{
			if contentV, e := utils.ReadNNI(data.Content); e == nil {
				fi.Size = uint64(contentV)
			}
		}
	case e = <-channelError:
	}

	return fi, e
}

// ReadAt Express Interest type: READAT, to get Data with Content bytes from file.
// Always request the same packets no matter the offset in file. Round down to
// the nearest multiple of MaxPayloadSize
func (consumer *Consumer) ReadAt(b []byte, off int64, filepath string) (n int, e error) {
	log.Debug("Read@", off, " ", len(b), " bytes from: ", filepath)

	var npkts = 0

	for byteOffsetV := (off / namespace.MaxPayloadSize) * namespace.MaxPayloadSize;
		byteOffsetV < off+int64(len(b)); byteOffsetV += namespace.MaxPayloadSize {

		name := ndn.ParseName(namespace.NamePrefixUriFileRead + filepath)
		name = append(
			name,
			ndn.NameComponent{Element: tlv.MakeElementNNI(an.TtByteOffsetNameComponent, byteOffsetV)},
		)

		consumer.expressInterest(name)
		npkts++
	}

	for i := 0; i < npkts; i++ {
		select {
		case data := <-channelData:
			{
				if len(data.Content) == 0 {
					continue
				}

				byteOffsetV, e := utils.ReadNNI(data.Name.Get(-1).Value)
				if e != nil {
					return n, e
				}

				if int64(byteOffsetV) < off { // Trim first segment
					n += copy(b, data.Content[off-int64(byteOffsetV):])
				} else {
					n += copy(b[int64(byteOffsetV)-off:], data.Content)
				}
			}
		case <-time.After(4 * time.Second):
			log.Error("Timeout")
		case e = <-channelError:
			return n, e
		}
	}

	return n, nil
}

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
	"sandie-ndn/go/internal/pkg/pipeline"
	"sandie-ndn/go/internal/pkg/utils"
	"time"
)

// Consumer
type Consumer struct {
	face       l3.Face
	mgmtClient *gqlmgmt.Client
	pipeline   pipeline.Pipeline
}

type FileInfo struct {
	size uint64
}

func FileInfoGetSize(info *FileInfo) uint64 {
	return info.size
}

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

	consumer.pipeline, _ = pipeline.NewFixed(consumer.face)
	return consumer, nil
}

// Close consumer
func (consumer *Consumer) Close() {
	log.Debug("Closing consumer")

	consumer.pipeline.Close()
	close(consumer.face.Tx())

	if consumer.mgmtClient != nil {
		time.Sleep(100 * time.Millisecond) // allow time to close face
		_ = consumer.mgmtClient.Close()
	}
}

// Stat Express Interest type: FILEINFO, for getting Data with Content FileInfo struct for file
func (consumer *Consumer) Stat(filepath string) (*FileInfo, error) {
	log.Debug("Stat: ", filepath)
	name := ndn.ParseName(namespace.NamePrefixUriFileinfo + filepath)

	consumer.pipeline.Send() <- ndn.MakeInterest(name, namespace.DefaultInterestLifetime)

	select {
	case data := <-consumer.pipeline.Get():
		if data.ContentType == an.ContentNack {
			return nil, utils.ReadApplicationLevelNackContent(data)
		}

		content, e := utils.ReadNNI(data.Content)
		if e != nil {
			return nil, e
		}

		return &FileInfo{size: uint64(content)}, nil

	case e := <-consumer.pipeline.Error():
		return nil, e
	}
}

// ReadAt Express Interest type: READAT, to get Data with Content bytes from file.
// Always request the same packets no matter the offset in file. Round down to
// the nearest multiple of MaxPayloadSize
func (consumer *Consumer) ReadAt(b []byte,
	off int64,
	filepath string,
) (n int, e error) {
	log.Debug("Read@", off, " ", len(b), " bytes from: ", filepath)

	var npkts = 0

	for byteOffsetV := (off / namespace.MaxPayloadSize) * namespace.MaxPayloadSize;
		byteOffsetV < off+int64(len(b)); byteOffsetV += namespace.MaxPayloadSize {

		name := ndn.ParseName(namespace.NamePrefixUriFileRead + filepath)
		name = append(
			name,
			ndn.NameComponent{Element: tlv.MakeElementNNI(an.TtByteOffsetNameComponent, byteOffsetV)},
		)

		consumer.pipeline.Send() <- ndn.MakeInterest(name, namespace.DefaultInterestLifetime)
		npkts++
	}

	for i := 0; i < npkts; i++ {
		select {
		case data := <-consumer.pipeline.Get():
			if len(data.Content) == 0 {
				continue
			}

			if data.ContentType == an.ContentNack {
				return n, utils.ReadApplicationLevelNackContent(data)
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

		case e = <-consumer.pipeline.Error():
			return n, e
		}
	}

	return n, nil
}

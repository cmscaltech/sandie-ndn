package xrdndndpdkproducer

/*
#include "xrdndndpdk-producer.h"
*/
import "C"
import (
	"fmt"
	"time"
	"unsafe"

	"ndn-dpdk/app/inputdemux"
	"ndn-dpdk/appinit"
	"ndn-dpdk/container/pktqueue"
	"ndn-dpdk/dpdk"
	"ndn-dpdk/iface"
)

// Producer instance and thread.
type Producer struct {
	dpdk.ThreadBase
	c *C.Producer
}

// NewProducer object
func NewProducer(face iface.IFace, producerSettings ProducerSettings) (producer *Producer, e error) {
	faceID := face.GetFaceId()
	socket := face.GetNumaSocket()

	producer = new(Producer)
	producer.c = (*C.Producer)(dpdk.Zmalloc("Producer", C.sizeof_Producer, socket))

	if _, e := pktqueue.NewAt(unsafe.Pointer(&producer.c.rxQueue), producerSettings.RxQueue, fmt.Sprintf("PingServer%d_rxQ", faceID), socket); e != nil {
		dpdk.Free(producer.c)
		return nil, nil
	}

	producer.c.dataMp = (*C.struct_rte_mempool)(appinit.MakePktmbufPool(appinit.MP_DATA0, socket).GetPtr())
	producer.c.face = (C.FaceId)(faceID)

	producer.ResetThreadBase()
	dpdk.InitStopFlag(unsafe.Pointer(&producer.c.stop))

	producer.SetProducerSettings(producerSettings)
	return producer, nil
}

// SetProducerSettings instance
func (producer *Producer) SetProducerSettings(producerSettings ProducerSettings) (e error) {
	producer.c.freshnessPeriod = C.uint32_t(producerSettings.FreshnessPeriod.Duration() / time.Millisecond)
	return nil
}

// GetRxQueue from Producer instance
func (producer *Producer) GetRxQueue() pktqueue.PktQueue {
	return pktqueue.FromPtr(unsafe.Pointer(&producer.c.rxQueue))
}

// ConfigureDemux for Producer instance
func (producer *Producer) ConfigureDemux(demux3 inputdemux.Demux3) {
	demuxI := demux3.GetInterestDemux()
	demuxI.InitFirst()
	demuxI.SetDest(0, producer.GetRxQueue())
}

// Start the thread.
func (producer *Producer) Start() error {
	return producer.LaunchImpl(func() int {
		C.Producer_Run(producer.c)
		return 0
	})
}

// Stop the thread.
func (producer *Producer) Stop() error {
	return producer.StopImpl(dpdk.NewStopFlag(unsafe.Pointer(&producer.c.stop)))
}

// Close the producer.
// The thread must be stopped before calling this.
func (producer *Producer) Close() (e error) {
	if e = producer.GetRxQueue().Close(); e != nil {
		return e
	}

	dpdk.Free(producer.c)
	return nil
}

package xrdndndpdkproducer

/*
#include "xrdndndpdk-producer.h"
*/
import "C"
import (
	"fmt"
	"time"
	"unsafe"

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

func newProducer(face iface.IFace, cfg ProducerSettings) (producer *Producer, e error) {
	faceId := face.GetFaceId()
	socket := face.GetNumaSocket()
	producerC := (*C.Producer)(dpdk.Zmalloc("Producer", C.sizeof_Producer, socket))

	var rxQueueAux pktqueue.Config

	if _, e := pktqueue.NewAt(unsafe.Pointer(&producerC.rxQueue), rxQueueAux,
		fmt.Sprintf("PingServer%d_rxQ", faceId), socket); e != nil {
		dpdk.Free(producerC)
		return nil, nil
	}

	producerC.dataMp = (*C.struct_rte_mempool)(appinit.MakePktmbufPool(
		appinit.MP_DATA0, socket).GetPtr())
	producerC.face = (C.FaceId)(faceId)

	producer = new(Producer)
	producer.c = producerC
	producer.ResetThreadBase()
	dpdk.InitStopFlag(unsafe.Pointer(&producerC.stop))

	if e := producer.ConfigureProducer(cfg); e != nil {
		return nil, fmt.Errorf("cfg: %s", e)
	}

	return producer, nil
}

func (producer *Producer) ConfigureProducer(cfg ProducerSettings) (e error) {
	producer.c.freshnessPeriod = C.uint32_t(cfg.FreshnessPeriod / time.Millisecond)
	return nil
}

func (producer *Producer) GetRxQueue() pktqueue.PktQueue {
	return pktqueue.FromPtr(unsafe.Pointer(&producer.c.rxQueue))
}

// Launch the thread.
func (producer *Producer) Launch() error {
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
func (producer *Producer) Close() error {
	producer.GetRxQueue().Close()
	dpdk.Free(producer.c)
	return nil
}

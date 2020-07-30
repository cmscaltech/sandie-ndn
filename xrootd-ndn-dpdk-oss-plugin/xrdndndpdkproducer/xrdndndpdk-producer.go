package xrdndndpdkproducer

/*
#include "xrdndndpdk-producer.h"
*/
import "C"
import (
	"sandie-ndn/xrootd-ndn-dpdk-oss-plugin/xrdndndpdkfilesystem"
	"time"
	"unsafe"

	"github.com/usnistgov/ndn-dpdk/core/cptr"
	"github.com/usnistgov/ndn-dpdk/dpdk/eal"
	"github.com/usnistgov/ndn-dpdk/dpdk/ealthread"
	"github.com/usnistgov/ndn-dpdk/iface"
	"github.com/usnistgov/ndn-dpdk/ndni"
)

// Producer instance and thread
type Producer struct {
	ealthread.Thread
	c          *C.Producer
	fileSystem unsafe.Pointer
}

// NewProducer object
func NewProducer(face iface.Face, settings ProducerSettings) (producer *Producer, e error) {
	faceID := face.ID()
	socket := face.NumaSocket()
	producer = new(Producer)
	producer.c = (*C.Producer)(eal.Zmalloc("Producer", C.sizeof_Producer, socket))

	if producer.fileSystem, e = xrdndndpdkfilesystem.GetFilesystem(settings.FilesystemType); e != nil {
		eal.Free(producer.c)
		return nil, e
	}

	settings.RxQueue.DisableCoDel = true
	if e := iface.PktQueueFromPtr(unsafe.Pointer(&producer.c.rxQueue)).Init(settings.RxQueue, socket); e != nil {
		eal.Free(producer.c)
		return nil, e
	}

	producer.c.payloadMp = (*C.struct_rte_mempool)(ndni.PayloadMempool.MakePool(socket).Ptr())
	producer.c.face = (C.FaceID)(faceID)

	producer.Thread = ealthread.New(
		cptr.Func0.C(unsafe.Pointer(C.Producer_Run), unsafe.Pointer(producer.c)),
		ealthread.InitStopFlag(unsafe.Pointer(&producer.c.stop)),
	)

	producer.Configure(settings)
	return producer, nil
}

// Configure producer instance
func (producer *Producer) Configure(settings ProducerSettings) (e error) {
	producer.c.fs = producer.fileSystem
	producer.c.freshnessPeriod = C.uint32_t(settings.FreshnessPeriod.Duration() / time.Millisecond)
	return nil
}

// RxQueue from Producer instance
func (producer *Producer) RxQueue() *iface.PktQueue {
	return iface.PktQueueFromPtr(unsafe.Pointer(&producer.c.rxQueue))
}

// ConfigureDemux for Producer instance
func (producer *Producer) ConfigureDemux(demuxI, demuxD, demuxN *iface.InputDemux) {
	demuxI.InitRoundrobin(1)
	q := producer.RxQueue()
	demuxI.SetDest(0, q)
	demuxD.InitToken()
	demuxN.InitToken()
	demuxD.SetDest(0, q)
	demuxN.SetDest(0, q)
}

// Close the producer
// The thread must be stopped before calling this.
func (producer *Producer) Close() error {
	producer.Stop()
	producer.RxQueue().Close()
	eal.Free(producer.c)

	xrdndndpdkfilesystem.FreeFilesystem(producer.fileSystem)
	return nil
}

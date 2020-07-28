package xrdndndpdkconsumer

/*
#include <math.h>
#include "xrdndndpdk-consumer-rx.h"
#include "xrdndndpdk-consumer-tx.h"
#include "../xrdndndpdk-common/xrdndndpdk-namespace.h"
*/
import "C"
import (
	"fmt"
	"unsafe"

	"github.com/usnistgov/ndn-dpdk/core/cptr"
	"github.com/usnistgov/ndn-dpdk/dpdk/eal"
	"github.com/usnistgov/ndn-dpdk/dpdk/ealthread"
	"github.com/usnistgov/ndn-dpdk/dpdk/ringbuffer"
	"github.com/usnistgov/ndn-dpdk/iface"
	"github.com/usnistgov/ndn-dpdk/ndn"
	"github.com/usnistgov/ndn-dpdk/ndni"
)

// Consumer Instance
type Consumer struct {
	Rx ealthread.Thread
	Tx ealthread.Thread

	rxC *C.ConsumerRx
	txC *C.ConsumerTx
}

// MessageRx struct between Go and C (Rx Thread/s)
type MessageRx struct {
	Error      bool
	Value      int64
	SegmentNum int64
}

type Counters struct {
	nInterest int64
	nData     int64
	nNack     int64
	nBytes    int64
}

var messagesRx chan MessageRx

func newConsumer(face iface.Face, settings ConsumerSettings) (*Consumer, error) {
	socket := face.NumaSocket()
	rxC := (*C.ConsumerRx)(eal.Zmalloc("ConsumerRx", C.sizeof_ConsumerRx, socket))

	settings.RxQueue.DisableCoDel = true
	if e := iface.PktQueueFromPtr(unsafe.Pointer(&rxC.rxQueue)).Init(settings.RxQueue, socket); e != nil {
		eal.Free(rxC)
		return nil, e
	}

	txC := (*C.ConsumerTx)(eal.Zmalloc("ConsumerTx", C.sizeof_ConsumerTx, socket))
	txC.face = (C.FaceID)(face.ID())
	txC.interestMp = (*C.struct_rte_mempool)(ndni.InterestMempool.MakePool(socket).Ptr())
	C.NonceGen_Init(&txC.nonceGen)

	var consumer Consumer
	consumer.rxC = rxC
	consumer.txC = txC

	consumer.Rx = ealthread.New(
		cptr.Func0.C(unsafe.Pointer(C.ConsumerRx_Run), unsafe.Pointer(rxC)),
		ealthread.InitStopFlag(unsafe.Pointer(&rxC.stop)),
	)
	consumer.Tx = ealthread.New(
		cptr.Func0.C(unsafe.Pointer(C.ConsumerTx_Run), unsafe.Pointer(txC)),
		ealthread.InitStopFlag(unsafe.Pointer(&txC.stop)),
	)

	if e := consumer.Configure(settings); e != nil {
		eal.Free(rxC)
		eal.Free(txC)
		return nil, fmt.Errorf("settings: %s", e)
	}

	requestQueue, e := ringbuffer.New(1024, socket, ringbuffer.ProducerMulti, ringbuffer.ConsumerSingle)
	if e != nil {
		return nil, fmt.Errorf("ringbuffer.New: %w", e)
	}
	consumer.txC.requestQueue = (*C.struct_rte_ring)(requestQueue.Ptr())

	messagesRx = make(chan MessageRx)
	return &consumer, nil
}

func (consumer *Consumer) Configure(settings ConsumerSettings) error {
	fileInfoPrefix := ndn.ParseName(C.PACKET_NAME_PREFIX_URI_FILEINFO)
	readPrefix := ndn.ParseName(C.PACKET_NAME_PREFIX_URI_READ)

	tplArgsFileInfo := []interface{}{fileInfoPrefix}
	tplArgsRead := []interface{}{readPrefix}
	if settings.MustBeFresh {
		tplArgsFileInfo = append(tplArgsFileInfo, ndn.MustBeFreshFlag)
		tplArgsRead = append(tplArgsRead, ndn.MustBeFreshFlag)
	}
	if settings.InterestLifetime != 0 {
		tplArgsFileInfo = append(tplArgsFileInfo, settings.InterestLifetime)
		tplArgsRead = append(tplArgsRead, settings.InterestLifetime)
	}

	ndni.InterestTemplateFromPtr(unsafe.Pointer(&consumer.txC.fileInfoPrefixTpl)).Init(tplArgsFileInfo...)
	ndni.InterestTemplateFromPtr(unsafe.Pointer(&consumer.txC.readPrefixTpl)).Init(tplArgsRead...)

	C.ConsumerRx_RegisterGoCallbacks(consumer.rxC)
	C.ConsumerTx_RegisterGoCallbacks(consumer.txC)
	consumer.ResetCounters()

	return nil
}

func (consumer *Consumer) GetFace() iface.Face {
	return iface.Get(iface.ID(consumer.txC.face))
}

// RxQueue from Consumer instance
func (consumer *Consumer) RxQueue() *iface.PktQueue {
	return iface.PktQueueFromPtr(unsafe.Pointer(&consumer.rxC.rxQueue))
}

// ConfigureDemux for Consumer instance
func (consumer *Consumer) ConfigureDemux(demuxI, demuxD, demuxN *iface.InputDemux) {
	demuxD.InitFirst()
	demuxN.InitFirst()
	q := consumer.RxQueue()
	demuxD.SetDest(0, q)
	demuxN.SetDest(0, q)
}

func (consumer *Consumer) SetLCores(rxLCore, txLCore eal.LCore) {
	consumer.Rx.SetLCore(rxLCore)
	consumer.Tx.SetLCore(txLCore)
}

// Launch the RX thread.
func (consumer *Consumer) Launch() {
	consumer.Rx.Launch()
	consumer.Tx.Launch()
}

// Stop Rx and TX threads.
func (consumer *Consumer) Stop() error {
	eTx := consumer.Tx.Stop()
	eRx := consumer.Rx.Stop()

	if eRx != nil || eTx != nil {
		return fmt.Errorf("RX %v; TX %v", eRx, eTx)
	}
	return nil
}

// Close the consumer.
// Both RX and TX threads must be stopped before calling this.
func (consumer *Consumer) Close() error {
	consumer.RxQueue().Close()
	eal.Free(consumer.rxC)
	ringbuffer.FromPtr(unsafe.Pointer(consumer.txC.requestQueue)).Close()
	eal.Free(consumer.txC)
	return nil
}

// ResetCounters Reset Rx and Tx Consumer thread counters for statistics
func (consumer *Consumer) ResetCounters() {
	C.ConsumerRx_ClearCounters(consumer.rxC)
	C.ConsumerTx_ClearCounters(consumer.txC)
}

// GetCounters Get Rx and Tx consumer counters
func (consumer *Consumer) GetCounters() (cnt Counters) {
	cnt.nInterest = int64(consumer.txC.cnt.nInterest)
	cnt.nData = int64(consumer.rxC.cnt.nData)
	cnt.nNack = int64(consumer.rxC.cnt.nNack)
	cnt.nBytes = int64(consumer.rxC.cnt.nBytes)
	return cnt
}

// RequestData over NDN
func (consumer *Consumer) RequestData(pathname string, count int64, offset int64, pt C.PacketType) int {
	name := ndn.ParseName(pathname)
	nameV, _ := name.MarshalBinary()

	var req C.ConsumerTxRequest
	req.nameL = C.uint16_t(copy(cptr.AsByteSlice(&req.nameV), nameV))
	req.pt = pt

	n := C.uint16_t(C.ceil(C.double(C.uint64_t(count)) / C.double(C.XRDNDNDPDK_MAX_PAYLOAD_SIZE)))
	req.npkts = n
	req.off = C.uint64_t(offset)

	C.rte_ring_enqueue(consumer.txC.requestQueue, (unsafe.Pointer)(&req))
	return int(n)
}

// FileInfo file
func (consumer *Consumer) FileInfo(pathname string) (filesize int64, e error) {
	consumer.RequestData(pathname, 0, 0, C.PACKET_FILEINFO)

	m := <-messagesRx
	if m.Error {
		return 0, fmt.Errorf("Return error code %d for fileinfo on file: %s", m.Value, pathname)
	}

	return m.Value, nil
}

// Read from file
func (consumer *Consumer) Read(pathname string, buf *C.uint8_t, count int64, offset int64) (nbytes int64, e error) {
	n := consumer.RequestData(pathname, count, offset, C.PACKET_READ)

	for i := 0; i < n; i++ {
		m := <-messagesRx

		if m.Error {
			return nbytes, fmt.Errorf("Return error code %d for read on file: %s", m.Value, pathname)
		}

		nbytes += m.Value
	}

	if nbytes > count {
		nbytes = count
	}

	return nbytes, nil
}

//export onContentCallback_Go
func onContentCallback_Go(value C.uint64_t, segmentNum C.uint64_t) {
	go func(channel chan<- MessageRx) {
		channel <- MessageRx{false, int64(value), int64(segmentNum)}
	}(messagesRx)
}

//export onErrorCallback_Go
func onErrorCallback_Go(errorCode C.uint64_t) {
	go func(channel chan<- MessageRx) {
		channel <- MessageRx{true, int64(errorCode), 0}
	}(messagesRx)
}

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

type Message struct {
	isError  bool // Nack Packet, Application Level Nack
	intValue C.uint64_t

	off     C.uint64_t
	content *C.struct_PContent
}

var messages chan Message

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

	messages = make(chan Message)
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

	C.registerRxCallbacks(consumer.rxC)
	C.registerTxCallbacks(consumer.txC)
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
	C.ConsumerRx_resetCounters(consumer.rxC)
	C.ConsumerTx_resetCounters(consumer.txC)
}

// RequestData over NDN
func (consumer *Consumer) RequestData(pathname string, buf *C.uint8_t,
	count C.uint64_t, off C.uint64_t, pt C.PacketType) (n C.uint16_t) {
	name := ndn.ParseName(pathname)
	nameV, _ := name.MarshalBinary()

	var req C.ConsumerTxRequest
	req.nameL = C.uint16_t(copy(cptr.AsByteSlice(&req.nameV), nameV))
	req.pt = pt
	req.npkts = C.uint16_t(C.ceil(C.double(count) / C.double(C.XRDNDNDPDK_PACKET_SIZE)))
	req.off = off

	C.rte_ring_enqueue(consumer.txC.requestQueue, (unsafe.Pointer)(&req))
	return req.npkts
}

// FileInfo file
func (consumer *Consumer) FileInfo(pathname string, buf *C.uint8_t) error {
	consumer.RequestData(pathname, buf, C.uint64_t(0), C.uint64_t(0), C.PACKET_FILEINFO)

	m := <-messages
	if m.isError {
		return fmt.Errorf("Return error code %d for fileinfo on file: %s", m.intValue, pathname)
	}

	defer C.rte_free(unsafe.Pointer(m.content.payload))
	defer C.rte_free(unsafe.Pointer(m.content))

	C.copyFromC(buf, 0, m.content.payload, m.content.offset, m.content.length)
	return nil
}

// Read from file
func (consumer *Consumer) Read(pathname string, buf *C.uint8_t,
	count C.uint64_t, off C.uint64_t) (nbytes C.uint64_t, e error) {
	n := consumer.RequestData(pathname, buf, count, off, C.PACKET_READ)

	nbytes = 0
	for i := C.uint16_t(0); i < n; i++ {
		m := <-messages

		if m.isError {
			return nbytes, fmt.Errorf("Return error code %d for read on file: %s", m.intValue, pathname)
		}

		nbytes += C.uint64_t(m.content.length)

		C.rte_free(unsafe.Pointer(m.content.payload))
		C.rte_free(unsafe.Pointer(m.content))

		// SC19: Don't copy content from C to Go Memory. We don't need to save file
		// C.copyFromC(buf, C.uint16_t((m.off-off)/C.XRDNDNDPDK_PACKET_SIZE), m.content.payload, m.content.offset, m.content.length)
	}

	return nbytes, nil
}

//export onContentCallback_Go
func onContentCallback_Go(content *C.struct_PContent, off C.uint64_t) {
	go func(channel chan<- Message) {
		channel <- Message{false, C.XRDNDNDPDK_ESUCCESS, off, content}
	}(messages)
}

//export onErrorCallback_Go
func onErrorCallback_Go(errorCode C.uint64_t) {
	go func(channel chan<- Message) {
		channel <- Message{true, errorCode, 0, nil}
	}(messages)
}

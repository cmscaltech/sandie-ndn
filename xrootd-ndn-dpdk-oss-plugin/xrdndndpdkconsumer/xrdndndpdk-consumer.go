package xrdndndpdkconsumer

/*
#include <math.h>
#include "xrdndndpdk-consumer-rx.h"
#include "xrdndndpdk-consumer-tx.h"
#include "../xrdndndpdk-common/xrdndndpdk-namespace.h"
*/
import "C"
import (
	"errors"
	"fmt"
	"unsafe"

	"ndn-dpdk/appinit"
	"ndn-dpdk/dpdk"
	"ndn-dpdk/iface"
	"ndn-dpdk/ndn"
)

// Consumer Instance
type Consumer struct {
	Rx ConsumerRxThread
	Tx ConsumerTxThread
}

// Consumer TX thread.
type ConsumerTxThread struct {
	dpdk.ThreadBase
	c *C.ConsumerTx

	FilePath string
}

// Consumer RX thread.
type ConsumerRxThread struct {
	dpdk.ThreadBase
	c *C.ConsumerRx
}

type Message struct {
	isError  bool // Nack Packet, Application Level Nack
	intValue C.uint64_t

	off     C.uint64_t
	content *C.struct_PContent
}

var messages chan Message

func newConsumer(face iface.IFace, settings ConsumerSettings) (consumer *Consumer, e error) {
	socket := face.GetNumaSocket()
	crC := (*C.ConsumerRx)(dpdk.Zmalloc("ConsumerRx", C.sizeof_ConsumerRx, socket))
	ctC := (*C.ConsumerTx)(dpdk.Zmalloc("ConsumerTx", C.sizeof_ConsumerTx, socket))
	ctC.face = (C.FaceId)(face.GetFaceId())
	ctC.interestMbufHeadroom = C.uint16_t(appinit.SizeofEthLpHeaders() + ndn.EncodeInterest_GetHeadroom())
	ctC.interestMp = (*C.struct_rte_mempool)(appinit.MakePktmbufPool(
		appinit.MP_INT, socket).GetPtr())

	C.NonceGen_Init(&ctC.nonceGen)

	consumer = new(Consumer)
	consumer.Rx.c = crC
	consumer.Rx.ResetThreadBase()
	dpdk.InitStopFlag(unsafe.Pointer(&crC.stop))

	consumer.Tx.c = ctC
	consumer.Tx.ResetThreadBase()
	dpdk.InitStopFlag(unsafe.Pointer(&ctC.stop))

	if e := consumer.ConfigureConsumer(settings); e != nil {
		return nil, fmt.Errorf("settings: %s", e)
	}

	messages = make(chan Message)

	return consumer, nil
}

func (consumer *Consumer) GetFace() iface.IFace {
	return iface.Get(iface.FaceId(consumer.Tx.c.face))
}

func (consumer *Consumer) ConfigureConsumer(settings ConsumerSettings) (e error) {
	if len(settings.FilePath) == 0 {
		e := errors.New("yaml: filepath not defined or empty string")
		return e
	}

	// Create template for open/fstat file system call Interest packets

	prefix, e := ndn.ParseName(C.XRDNDNDPDK_SYSCALL_PREFIX_URI)
	if e != nil {
		return e
	}

	prefixTpl := ndn.NewInterestTemplate()
	prefixTpl.SetNamePrefix(prefix)
	prefixTpl.SetMustBeFresh(settings.MustBeFresh)
	if settings.InterestLifetime != 0 {
		prefixTpl.SetInterestLifetime(settings.InterestLifetime)
	}

	if e = prefixTpl.CopyToC(unsafe.Pointer(&consumer.Tx.c.prefixTpl),
		unsafe.Pointer(&consumer.Tx.c.prefixTplBuf),
		unsafe.Sizeof(consumer.Tx.c.prefixTplBuf),
		unsafe.Pointer(&consumer.Tx.c.prefixBuffer),
		unsafe.Sizeof(consumer.Tx.c.prefixBuffer)); e != nil {
		return e
	}

	// Create template for read file system call Interest packets

	readPrefix, e := ndn.ParseName(C.XRDNDNDPDK_SYSCALL_PREFIX_URI +
		"/" + C.XRDNDNDPDK_SYSCALL_READ_URI + settings.FilePath)
	if e != nil {
		return e
	}

	readPrefixTpl := ndn.NewInterestTemplate()
	readPrefixTpl.SetNamePrefix(readPrefix)
	readPrefixTpl.SetMustBeFresh(settings.MustBeFresh)
	if settings.InterestLifetime != 0 {
		readPrefixTpl.SetInterestLifetime(settings.InterestLifetime)
	}

	if e = readPrefixTpl.CopyToC(unsafe.Pointer(&consumer.Tx.c.readPrefixTpl),
		unsafe.Pointer(&consumer.Tx.c.readPrefixTplBuf),
		unsafe.Sizeof(consumer.Tx.c.readPrefixTplBuf),
		unsafe.Pointer(&consumer.Tx.c.readBuffer),
		unsafe.Sizeof(consumer.Tx.c.readBuffer)); e != nil {
		return e
	}

	consumer.Tx.c.segmentNumberComponent.compT = C.TT_GenericNameComponent
	consumer.Tx.c.segmentNumberComponent.compL = C.uint8_t(C.sizeof_uint64_t)
	consumer.Tx.c.segmentNumberComponent.compV = C.uint64_t(0)

	consumer.Tx.FilePath = settings.FilePath
	C.registerRxCallbacks(consumer.Rx.c)
	C.registerTxCallbacks(consumer.Tx.c)

	C.ConsumerRx_resetCounters(consumer.Rx.c)
	C.ConsumerTx_resetCounters(consumer.Tx.c)

	return nil
}

// Launch the RX thread.
func (rx ConsumerRxThread) Launch() error {
	return rx.LaunchImpl(func() int {
		C.ConsumerRx_Run(rx.c)
		return 0
	})
}

// Stop the RX thread.
func (rx *ConsumerRxThread) Stop() error {
	return rx.StopImpl(dpdk.NewStopFlag(unsafe.Pointer(&rx.c.stop)))
}

// Stop the TX thread.
func (tx *ConsumerTxThread) Stop() error {
	return tx.StopImpl(dpdk.NewStopFlag(unsafe.Pointer(&tx.c.stop)))
}

// Close the consumer.
// Both RX and TX threads must be stopped before calling this.
func (consumer *Consumer) Close() error {
	dpdk.Free(consumer.Rx.c)
	dpdk.Free(consumer.Tx.c)
	return nil
}

// Open file
func (tx *ConsumerTxThread) OpenFile() (e error) {
	tx.Stop()
	e = tx.LaunchImpl(func() int {
		suffix, e := ndn.ParseName("/" + C.XRDNDNDPDK_SYSCALL_OPEN_URI + tx.FilePath)
		if e != nil {
			return -1
		}

		nameV := unsafe.Pointer(C.malloc(C.uint64_t(suffix.Size())))
		defer C.free(nameV)

		var lname C.LName
		e = suffix.CopyToLName(unsafe.Pointer(&lname), nameV, uintptr(suffix.Size()))
		if e != nil {
			return -1
		}

		C.ConsumerTx_sendInterest(tx.c, &lname)
		return 0
	})

	if e != nil {
		return e
	}

	m := <-messages

	if m.intValue != C.XRDNDNDPDK_ESUCCESS {
		return fmt.Errorf("Unable to open file: %s", tx.FilePath)
	}

	return nil
}

// Fstat file
func (tx *ConsumerTxThread) FstatFile(buf *C.uint8_t) (e error) {
	tx.Stop()
	e = tx.LaunchImpl(func() int {
		suffix, e := ndn.ParseName("/" + C.XRDNDNDPDK_SYSCALL_FSTAT_URI + tx.FilePath)
		if e != nil {
			return -1
		}

		nameV := unsafe.Pointer(C.malloc(C.uint64_t(suffix.Size())))
		defer C.free(nameV)

		var lname C.LName
		e = suffix.CopyToLName(unsafe.Pointer(&lname), nameV, uintptr(suffix.Size()))
		if e != nil {
			return -1
		}

		C.ConsumerTx_sendInterest(tx.c, &lname)
		return 0
	})

	if e != nil {
		return e
	}

	m := <-messages

	if m.isError {
		return fmt.Errorf("Unable to call fstat on file: %s", tx.FilePath)
	}

	defer C.rte_free(unsafe.Pointer(m.content.payload))
	defer C.rte_free(unsafe.Pointer(m.content))

	C.copyFromC(buf, 0, m.content.payload, m.content.offset, m.content.length)

	return nil
}

// Read from file
func (tx *ConsumerTxThread) Read(buf *C.uint8_t, count C.uint64_t, off C.uint64_t) (nbytes C.uint64_t, e error) {
	nInterests := C.uint16_t(C.ceil(C.double(count) / C.double(C.XRDNDNDPDK_PACKET_SIZE)))
	nbytes = 0

	tx.Stop()
	e = tx.LaunchImpl(func() int {
		C.ConsumerTx_sendInterests(tx.c, off, nInterests)
		return 0
	})

	if e != nil {
		return 0, e
	}

	for i := C.uint16_t(0); i < nInterests; i++ {
		m := <-messages

		if m.isError {
			return nbytes, fmt.Errorf("File offset read")
		}

		nbytes += m.content.length

		defer C.rte_free(unsafe.Pointer(m.content.payload))
		defer C.rte_free(unsafe.Pointer(m.content))

		C.copyFromC(buf, C.uint16_t((m.off-off)/C.XRDNDNDPDK_PACKET_SIZE), m.content.payload, m.content.offset, m.content.length)
	}

	return nbytes, e
}

//export onContentCallback_Go
func onContentCallback_Go(content *C.struct_PContent, off C.uint64_t) {
	go func(channel chan<- Message) {
		channel <- Message{false, C.XRDNDNDPDK_ESUCCESS, off, content}
	}(messages)
}

//export onNonNegativeIntegerCallback_Go
func onNonNegativeIntegerCallback_Go(retCode C.uint64_t) {
	go func(channel chan<- Message) {
		channel <- Message{false, retCode, 0, nil}
	}(messages)
}

//export onErrorCallback_Go
func onErrorCallback_Go(errorCode C.uint64_t) {
	go func(channel chan<- Message) {
		channel <- Message{true, errorCode, 0, nil}
	}(messages)
}

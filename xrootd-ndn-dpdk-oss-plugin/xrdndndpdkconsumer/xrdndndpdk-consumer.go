package xrdndndpdkconsumer

/*
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
	isError   bool // Nack Packet, Application Level Nack
	intValue  C.uint64_t
	byteValue []byte
}

var channel chan Message

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

	channel = make(chan Message)

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

	prefix, e := ndn.ParseName(C.XRDNDNDPDK_SYSCALL_PREFIX_URI)
	if e != nil {
		return e
	}

	tpl := ndn.NewInterestTemplate()
	tpl.SetNamePrefix(prefix)
	tpl.SetMustBeFresh(settings.MustBeFresh)
	if settings.InterestLifetime != 0 {
		tpl.SetInterestLifetime(settings.InterestLifetime)
	}

	if e = tpl.CopyToC(unsafe.Pointer(&consumer.Tx.c.tpl),
		unsafe.Pointer(&consumer.Tx.c.tplPrepareBuffer),
		unsafe.Sizeof(consumer.Tx.c.tplPrepareBuffer),
		unsafe.Pointer(&consumer.Tx.c.prefixBuffer),
		unsafe.Sizeof(consumer.Tx.c.prefixBuffer)); e != nil {
		return e
	}

	consumer.Tx.FilePath = settings.FilePath
	C.registerRxCallbacks(consumer.Rx.c)
	C.registerTxCallbacks(consumer.Tx.c)

	C.ConsumerRx_ResetCounters(consumer.Rx.c)

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

	m := <-channel

	if m.intValue != C.XRDNDNDPDK_ESUCCESS {
		return fmt.Errorf("Unable to open file: %s", tx.FilePath)
	}

	return nil
}

// Fstat file
func (tx *ConsumerTxThread) FstatFile() (data []byte, e error) {
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
		return nil, e
	}

	m := <-channel

	if m.isError {
		return nil, fmt.Errorf("Unable to call fstat on file: %s", tx.FilePath)
	}

	return m.byteValue, nil
}

//export onContentCallback_Go
func onContentCallback_Go(content *C.struct_PContent) {
	contentSlice := C.GoBytes(unsafe.Pointer(content.buff), C.int(content.length))
	go func() {
		channel <- Message{false, C.XRDNDNDPDK_ESUCCESS, contentSlice}
	}()
}

//export onNonNegativeIntegerCallback_Go
func onNonNegativeIntegerCallback_Go(retCode C.uint64_t) {
	go func() {
		channel <- Message{false, retCode, nil}
	}()
}

//export onErrorCallback_Go
func onErrorCallback_Go(errorCode C.uint64_t) {
	go func() {
		channel <- Message{true, errorCode, nil}
	}()
}

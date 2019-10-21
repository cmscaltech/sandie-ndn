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

var channelIntegerMessage chan C.uint64_t

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

	channelIntegerMessage = make(chan C.uint64_t)

	return consumer, nil
}

func (consumer *Consumer) GetFace() iface.IFace {
	return iface.Get(iface.FaceId(consumer.Tx.c.face))
}

func (consumer *Consumer) ConfigureConsumer(settings ConsumerSettings) (e error) {
	if len(settings.FilePath) == 0 {
		e := errors.New("yaml: filepath not defined or empty string\n")
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

	return nil
}

// Launch the RX thread.
func (rx ConsumerRxThread) Launch() error {
	return rx.LaunchImpl(func() int {
		fmt.Println("Rx Thread started")
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
		fmt.Println("Sending open file system call Interest from Tx")
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

		C.ConsumerTx_SendOpenInterest(tx.c, &lname) /******************************************************************************
		 * Created on Mon Oct 21 2019
		 *
		 * Copyright (c) 2019 California Institute of Technology
		 *****************************************************************************/

		return 0
	})

	if e != nil {
		return e
	}

	openRetCode := <-channelIntegerMessage
	if openRetCode != C.XRDNDNDPDK_ESUCCESS {
		return fmt.Errorf("Unable to open file: %s\n", tx.FilePath)
	} else {
		fmt.Println("Successfully opened file: ", tx.FilePath)
	}

	return nil
}

// //export onDataCallback_Go
// func onDataCallback_Go(content *C.struct_PContent) {
// 	fmt.Println("onDataCallback_Go")
// 	fmt.Println("Content offset: ", content.offset, " Content length: ", content.length)

// 	gSlice := (*[1 << 30]C.uint8_t)(unsafe.Pointer(content.buff))[:content.length]
// 	fmt.Println(gSlice)
// }

//export onOpenDataCallback_Go
func onOpenDataCallback_Go(openRetCode C.uint64_t) {
	fmt.Println("Rx callback to openData")
	go func() {
		channelIntegerMessage <- openRetCode
	}()
}

//export onErrorCallback_Go
func onErrorCallback_Go() {
	fmt.Println("Rx callback to error")
	go func() {
		channelIntegerMessage <- C.uint64_t(C.XRDNDNDPDK_EFAILURE)
	}()
}

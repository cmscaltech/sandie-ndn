package xrdndndpdkconsumer

/*
#include <sys/stat.h>
#include "../xrdndndpdk-common/xrdndndpdk-input.h"
#include "../xrdndndpdk-common/xrdndndpdk-namespace.h"
*/
import "C"
import (
	"errors"
	"fmt"
	"time"
	"unsafe"

	"github.com/cheggaaa/pb"

	"ndn-dpdk/appinit"
	"ndn-dpdk/dpdk"
	"ndn-dpdk/iface"
	"ndn-dpdk/iface/createface"
)

// LCoreAlloc roles.
const (
	LCoreRole_Input      = iface.LCoreRole_RxLoop
	LCoreRole_Output     = iface.LCoreRole_TxLoop
	LCoreRole_ConsumerRx = "ConsumerRx"
	LCoreRole_ConsumerTx = "ConsumerTx"
)

type App struct {
	task    Task
	rxls    []*iface.RxLoop
	initCfg InitConfig
}

func New(cfg TaskConfig, initCfg InitConfig) (app *App, e error) {
	app = new(App)
	app.initCfg = initCfg

	appinit.ProvideCreateFaceMempools()
	for _, numaSocket := range createface.ListRxTxNumaSockets() {
		// TODO create rxl and txl for configured faces only
		rxLCore := dpdk.LCoreAlloc.Alloc(LCoreRole_Input, numaSocket)
		rxl := iface.NewRxLoop(rxLCore.GetNumaSocket())
		rxl.SetLCore(rxLCore)
		app.rxls = append(app.rxls, rxl)
		createface.AddRxLoop(rxl)

		txLCore := dpdk.LCoreAlloc.Alloc(LCoreRole_Output, numaSocket)
		txl := iface.NewTxLoop(txLCore.GetNumaSocket())
		txl.SetLCore(txLCore)
		txl.Launch()
		createface.AddTxLoop(txl)
	}

	face, e := createface.Create(cfg.Face.Locator)
	if e != nil {
		return nil, fmt.Errorf("face creation error: %v", e)
	}

	task, e := newTask(face, cfg)
	if e != nil {
		return nil, fmt.Errorf("init error: %v", e)
	}
	app.task = task

	return app, nil
}

func (app *App) Launch() {
	for _, rxl := range app.rxls {
		app.launchRxl(rxl)
	}

	app.task.Launch()
}

func (app *App) launchRxl(rxl *iface.RxLoop) {
	hasFace := false
	minFaceId := iface.FACEID_MAX
	maxFaceId := iface.FACEID_MIN
	for _, faceId := range rxl.ListFaces() {
		hasFace = true
		if faceId < minFaceId {
			minFaceId = faceId
		}
		if faceId > maxFaceId {
			maxFaceId = faceId
		}
	}
	if !hasFace {
		return
	}

	inputC := C.Input_New(C.uint16_t(minFaceId),
		C.uint16_t(maxFaceId), C.unsigned(rxl.GetNumaSocket()))

	entryC := C.Input_GetEntry(inputC, C.uint16_t(app.task.Face.GetFaceId()))
	if entryC == nil {
		panic(errors.New("Could no get Input entry"))
	}
	if app.task.consumer != nil {
		queue, e := dpdk.NewRing(fmt.Sprintf("consumer-rx"), app.initCfg.QueueCapacity,
			app.task.Face.GetNumaSocket(), true, true)
		if e != nil {
			panic(e)
		}
		entryC.consumerQueue = (*C.struct_rte_ring)(queue.GetPtr())
		app.task.consumer.Rx.c.rxQueue = entryC.consumerQueue
	}

	rxl.SetCallback(unsafe.Pointer(C.Input_FaceRx), unsafe.Pointer(inputC))
	rxl.Launch()
}

type Task struct {
	Face     iface.IFace
	consumer *Consumer
}

func newTask(face iface.IFace, cfg TaskConfig) (task Task, e error) {
	numaSocket := face.GetNumaSocket()
	task.Face = face
	if cfg.Consumer != nil {
		if task.consumer, e = newConsumer(task.Face, *cfg.Consumer); e != nil {
			return Task{}, e
		}
		task.consumer.Rx.SetLCore(dpdk.LCoreAlloc.Alloc(LCoreRole_ConsumerRx, numaSocket))
		task.consumer.Tx.SetLCore(dpdk.LCoreAlloc.Alloc(LCoreRole_ConsumerTx, numaSocket))
	}
	return task, nil
}

func (task *Task) Launch() {
	if task.consumer != nil {
		task.consumer.Rx.Launch()
		// task.consumer.Tx.Launch()
	}
}

func (task *Task) Close() error {
	if task.consumer != nil {
		task.consumer.Close()
	}
	task.Face.Close()
	return nil
}

func (app *App) CopyFileOverNDN() (e error) {
	if app.task.consumer == nil {
		app.task.Close()
		return errors.New("nil consumer")
	}

	e = app.task.consumer.Tx.OpenFile()
	if e != nil {
		app.task.Close()
		return e
	}
	app.task.consumer.Tx.Stop()
	fmt.Println("-> successfully opened file:", app.task.consumer.Tx.FilePath)

	data, e := app.task.consumer.Tx.FstatFile()
	if e != nil {
		app.task.Close()
		return e
	}
	app.task.consumer.Tx.Stop()

	fileSize := C.uint64_t((*C.struct_stat)(unsafe.Pointer(&data[0])).st_size)
	fmt.Printf("-> file size: %d bytes\n", fileSize)

	chunkSize := C.uint64_t(C.XRDNDNDPDK_PACKET_SIZE * (C.CONSUMER_MAX_BURST_SIZE - 2))
	bufSize := C.uint64_t(0)

	pBar := pb.New(int(fileSize))
	pBar.ShowPercent = true
	pBar.ShowCounters = true
	pBar.ShowTimeLeft = false
	pBar.ShowFinalTime = false
	pBar.SetMaxWidth(100)
	pBar.Prefix("-> bytes:")

	pBar.Start()
	start := time.Now()

	for i := C.uint64_t(0); i < fileSize; i += chunkSize {
		if fileSize-i < chunkSize {
			bufSize = fileSize - i
		} else {
			bufSize = chunkSize
		}

		data, e := app.task.consumer.Tx.Read(i, bufSize)
		if e != nil {
			app.task.Close()
			return e
		}

		app.task.consumer.Tx.Stop()
		pBar.Add(len(data))
	}

	elapsed := time.Since(start)
	pBar.Finish()

	fmt.Printf("---------------------------------------------------------\n")
	fmt.Printf("--                 TRANSFER FILE REPORT                --\n")
	fmt.Printf("---------------------------------------------------------\n")
	fmt.Printf("-- Interest Packets : %d\n", app.task.consumer.Tx.c.nInterests)
	fmt.Printf("-- Data Packets     : %d\n", app.task.consumer.Rx.c.nData)
	fmt.Printf("-- Nack Packets     : %d\n", app.task.consumer.Rx.c.nNacks)
	fmt.Printf("-- Errors           : %d\n\n", app.task.consumer.Rx.c.nErrors)
	fmt.Printf("-- Maximum Packet size : %d Bytes\n", C.XRDNDNDPDK_PACKET_SIZE)
	fmt.Printf("-- Read chunk size     : %d Bytes (%d packets)\n", chunkSize, C.CONSUMER_MAX_BURST_SIZE-2)
	fmt.Printf("-- Bytes transmitted   : %d Bytes\n", app.task.consumer.Rx.c.nBytes)
	fmt.Printf("-- Time elapsed        : %s\n\n", elapsed)
	fmt.Printf("-- Throughput          : %.4f Mbit/s \n", (((float64(app.task.consumer.Rx.c.nBytes)/1024)/1024)*8)/elapsed.Seconds())
	fmt.Printf("---------------------------------------------------------\n")
	fmt.Printf("---------------------------------------------------------\n")

	// TODO: Remove this sleep
	time.Sleep(5 * time.Second)
	app.task.Close()
	return nil
}

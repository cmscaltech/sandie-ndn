package xrdndndpdkconsumer

/*
#include <sys/stat.h>
#include <stdlib.h>
#include "../xrdndndpdk-common/xrdndndpdk-namespace.h"
*/
import "C"
import (
	"errors"
	"fmt"
	"time"
	"unsafe"

	"github.com/cheggaaa/pb"

	"ndn-dpdk/app/inputdemux"
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
	inputs  []*Input
	initCfg InitConfig
}

type Input struct {
	rxl    *iface.RxLoop
	demux3 inputdemux.Demux3
}

func New(cfg TaskConfig, initCfg InitConfig) (app *App, e error) {
	app = new(App)
	app.initCfg = initCfg
	appinit.ProvideCreateFaceMempools()

	createface.CustomGetRxl = func(rxg iface.IRxGroup) *iface.RxLoop {
		lc := dpdk.LCoreAlloc.Alloc(LCoreRole_Input, rxg.GetNumaSocket())
		socket := lc.GetNumaSocket()
		rxl := iface.NewRxLoop(socket)
		rxl.SetLCore(lc)

		var input Input
		input.rxl = rxl
		app.inputs = append(app.inputs, &input)

		createface.AddRxLoop(rxl)
		return rxl
	}

	createface.CustomGetTxl = func(face iface.IFace) *iface.TxLoop {
		lc := dpdk.LCoreAlloc.Alloc(LCoreRole_Output, face.GetNumaSocket())
		socket := lc.GetNumaSocket()
		txl := iface.NewTxLoop(socket)
		txl.SetLCore(lc)
		txl.Launch()

		createface.AddTxLoop(txl)
		return txl
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
	for _, input := range app.inputs {
		app.launchInput(input)
	}

	app.task.Launch()
}

func (app *App) launchInput(input *Input) {
	faces := input.rxl.ListFaces()
	if len(faces) != 1 {
		panic("RxLoop should have exactly one face")
	}

	input.demux3 = inputdemux.NewDemux3(input.rxl.GetNumaSocket())
	input.demux3.GetInterestDemux().InitDrop()
	input.demux3.GetDataDemux().InitDrop()
	input.demux3.GetNackDemux().InitDrop()

	app.task.ConfigureDemux(input.demux3)
	input.rxl.SetCallback(inputdemux.Demux3_FaceRx, input.demux3.GetPtr())
	input.rxl.Launch()
}

type Task struct {
	Face     iface.IFace
	consumer *Consumer
	files    []string
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

	if len(cfg.Files) == 0 {
		return Task{}, errors.New("yaml: list of file paths empty or not defined")
	}

	task.files = cfg.Files
	return task, nil
}

func (task *Task) ConfigureDemux(demux3 inputdemux.Demux3) {
	demuxD := demux3.GetDataDemux()
	demuxN := demux3.GetNackDemux()

	demuxD.InitFirst()
	demuxN.InitFirst()
	q := task.consumer.GetRxQueue()
	demuxD.SetDest(0, q)
	demuxN.SetDest(0, q)

}

func (task *Task) Launch() {
	if task.consumer != nil {
		task.consumer.Rx.Launch()
		// task.consumer.Tx.Launch()
	}
}

func (task *Task) Close() error {
	if task.consumer != nil {
		task.consumer.Tx.Stop()
		task.consumer.Rx.Stop()
		task.consumer.Close()
	}
	task.Face.Close()
	return nil
}

func (app *App) Run() (e error) {
	for index, path := range app.task.files {
		fmt.Printf("\n--> Copy file [%d]: %s\n", index, path)
		e = app.task.CopyFileOverNDN(path)

		app.task.consumer.ResetCounters()
	}

	app.task.Close()
	return nil
}

func (task *Task) CopyFileOverNDN(path string) (e error) {
	if task.consumer == nil {
		return errors.New("nil consumer")
	}

	// Get file stat
	stat := (*C.uint8_t)(C.malloc(C.sizeof_struct_stat))
	e = task.consumer.Tx.FileInfo(stat, path)
	if e != nil {
		return e
	}

	filesz := C.uint64_t((*C.struct_stat)(unsafe.Pointer(stat)).st_size)
	C.free(unsafe.Pointer(stat))

	fmt.Printf("-> file size: %d bytes\n", filesz)

	// Read file
	maxCount := C.uint64_t(C.XRDNDNDPDK_PACKET_SIZE * (C.CONSUMER_MAX_BURST_SIZE - 2))
	count := C.uint64_t(0)

	pBar := pb.New(int(filesz))
	pBar.ShowPercent = true
	pBar.ShowCounters = true
	pBar.ShowTimeLeft = false
	pBar.ShowFinalTime = false
	pBar.SetMaxWidth(100)
	pBar.Prefix("-> bytes:")

	// SC19: This buffer will not be used. We don't need to save file
	buf := (*C.uint8_t)(C.malloc(maxCount * C.sizeof_uint8_t))
	defer C.free(unsafe.Pointer(buf))

	var samplingStart time.Time
	var samplingEnd time.Duration
	samplingCount := C.uint64_t(0)
	sampling := false

	pBar.Start()
	start := time.Now()

	for off := C.uint64_t(0); off < filesz; off += maxCount {
		if filesz-off < maxCount {
			count = filesz - off
		} else {
			count = maxCount
		}

		if !sampling && off > filesz/2 {
			sampling = true
			samplingStart = time.Now()
		}

		ret, e := task.consumer.Tx.Read(path, buf, count, off)
		if e != nil {
			return e
		}

		if sampling {
			samplingCount += ret
		}

		pBar.Add(int(ret))
	}

	if sampling {
		samplingEnd = time.Since(samplingStart)
	}

	elapsed := time.Since(start)
	pBar.Finish()

	task.consumer.Tx.Stop()

	fmt.Printf("---------------------------------------------------------\n")
	fmt.Printf("--                 TRANSFER FILE REPORT                --\n")
	fmt.Printf("---------------------------------------------------------\n")
	fmt.Printf("-- Interest Packets : %d\n", task.consumer.Tx.c.nInterests)
	fmt.Printf("-- Data Packets     : %d\n", task.consumer.Rx.c.nData)
	fmt.Printf("-- Nack Packets     : %d\n", task.consumer.Rx.c.nNacks)
	fmt.Printf("-- Errors           : %d\n\n", task.consumer.Rx.c.nErrors)
	fmt.Printf("-- Maximum Packet size  : %d Bytes\n", C.XRDNDNDPDK_PACKET_SIZE)
	fmt.Printf("-- Read chunk size      : %d Bytes (%d packets)\n", maxCount, C.CONSUMER_MAX_BURST_SIZE-2)
	fmt.Printf("-- Bytes transmitted    : %d Bytes\n", task.consumer.Rx.c.nBytes)
	fmt.Printf("-- Time elapsed         : %s\n\n", elapsed)
	fmt.Printf("-- Goodput              : %.4f Mbit/s \n", (((float64(pBar.Get())/1024)/1024)*8)/elapsed.Seconds())
	fmt.Printf("-- Throughput (2nd 1/2) : %.4f Mbit/s \n", (((float64(samplingCount)/1024)/1024)*8)/samplingEnd.Seconds())
	fmt.Printf("-- Throughput           : %.4f Mbit/s \n", (((float64(task.consumer.Rx.c.nBytes)/1024)/1024)*8)/elapsed.Seconds())
	fmt.Printf("---------------------------------------------------------\n")
	fmt.Printf("---------------------------------------------------------\n")

	return nil
}

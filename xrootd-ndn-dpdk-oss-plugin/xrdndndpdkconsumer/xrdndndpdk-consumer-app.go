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
	"github.com/usnistgov/ndn-dpdk/dpdk/ealthread"
	"github.com/usnistgov/ndn-dpdk/iface"
	"github.com/usnistgov/ndn-dpdk/iface/createface"
)

// LCoreAlloc roles
const (
	LCoreRole_Input      = "RX"
	LCoreRole_Output     = "TX"
	LCoreRole_ConsumerRx = "ConsumerRx"
	LCoreRole_ConsumerTx = "ConsumerTx"
)

// App struct
type App struct {
	face     iface.Face
	consumer *Consumer
	files    []string
	inputs   []*Input
}

// Input struct
type Input struct {
	rxl iface.RxLoop
}

// New App object instance
func New(initCfgConsumer InitConfigConsumer) (app *App, e error) {
	app = new(App)

	iface.ChooseRxLoop = func(rxg iface.RxGroup) iface.RxLoop {
		rxl := iface.NewRxLoop(rxg.NumaSocket())
		ealthread.AllocThread(rxl)

		var input Input
		input.rxl = rxl
		app.inputs = append(app.inputs, &input)
		return rxl
	}

	iface.ChooseTxLoop = func(face iface.Face) iface.TxLoop {
		txl := iface.NewTxLoop(face.NumaSocket())
		ealthread.Launch(txl)
		return txl
	}

	app.face, e = createface.Create(initCfgConsumer.Face.Locator)
	if e != nil {
		return nil, fmt.Errorf("face creation error: %v", e)
	}

	if initCfgConsumer.Consumer == nil {
		return nil, fmt.Errorf("Init config for consumer is empty")
	}

	if len(initCfgConsumer.Files) == 0 {
		return nil, errors.New("yaml: list of file paths empty or not defined")
	} else {
		app.files = initCfgConsumer.Files
	}

	socket := app.face.NumaSocket()
	if app.consumer, e = newConsumer(app.face, *initCfgConsumer.Consumer); e != nil {
		return nil, e
	}

	if app.consumer == nil {
		return nil, fmt.Errorf("Unable to get Consumer object")
	}

	app.consumer.SetLCores(ealthread.DefaultAllocator.Alloc(LCoreRole_ConsumerRx, socket),
		ealthread.DefaultAllocator.Alloc(LCoreRole_ConsumerTx, socket))
	return app, nil
}

// Launch App
func (app *App) Launch() {
	for _, input := range app.inputs {
		app.launchInput(input)
	}

	app.consumer.Launch()
}

// LaunchInput
func (app *App) launchInput(input *Input) {
	demuxI := input.rxl.InterestDemux()
	demuxD := input.rxl.DataDemux()
	demuxN := input.rxl.NackDemux()

	demuxI.InitDrop()
	demuxD.InitDrop()
	demuxN.InitDrop()

	app.consumer.ConfigureDemux(demuxI, demuxD, demuxN)
	input.rxl.Launch()
}

// Close App
func (app *App) Close() error {
	if app.consumer != nil {
		app.consumer.Stop()
		app.consumer.Close()
	}

	app.face.Close()
	return nil
}

// Run App
func (app *App) Run() (e error) {
	if app.consumer == nil {
		return errors.New("nil consumer")
	}

	for index, path := range app.files {
		fmt.Printf("\n--> Copy file [%d]: %s\n", index, path)
		if e = app.CopyFileOverNDN(path); e != nil {
			fmt.Println(e)
			continue
		}
	}

	app.Close()
	return nil
}

func (app *App) CopyFileOverNDN(path string) (e error) {
	app.consumer.ResetCounters()

	// Get file size
	var fileSize C.uint64_t
	{
		stat := (*C.uint8_t)(C.malloc(C.sizeof_struct_stat))
		defer C.free(unsafe.Pointer(stat))

		if e = app.consumer.FileInfo(path, stat); e != nil {
			return e
		}
		fileSize = C.uint64_t((*C.struct_stat)(unsafe.Pointer(stat)).st_size)
		fmt.Printf("-> File Size: %d B\n", fileSize)
	}

	pBar := pb.New(int(fileSize))
	pBar.ShowPercent = true
	pBar.ShowCounters = true
	pBar.ShowTimeLeft = false
	pBar.ShowFinalTime = false
	pBar.SetMaxWidth(100)
	pBar.Prefix("-> bytes:")
	pBar.Start()

	// Read file
	bufferSize := C.uint64_t(1048576) // 1MB

	start := time.Now()
	for offset := C.uint64_t(0); offset < fileSize; offset += bufferSize {
		nBytes := C.uint64_t(0)

		if bufferSize > fileSize {
			nBytes = fileSize
		} else if fileSize-offset < bufferSize {
			nBytes = fileSize - offset
		} else {
			nBytes = bufferSize
		}

		//buf := (*C.uint8_t)(C.malloc(nBytes * C.sizeof_uint8_t))
		retRead, e := app.consumer.Read(path, nil, nBytes, offset)
		// C.free(unsafe.Pointer(buf))

		if e != nil {
			return e
		}

		pBar.Add(int(retRead))
	}

	elapsed := time.Since(start)
	pBar.Finish()

	fmt.Printf("---------------------------------------------------------\n")
	fmt.Printf("--                 TRANSFER FILE REPORT                --\n")
	fmt.Printf("---------------------------------------------------------\n")
	fmt.Printf("-- Interest Packets : %d\n", app.consumer.txC.nInterests)
	fmt.Printf("-- Data Packets     : %d\n", app.consumer.rxC.nData)
	fmt.Printf("-- Nack Packets     : %d\n", app.consumer.rxC.nNacks)
	fmt.Printf("-- Errors           : %d\n\n", app.consumer.rxC.nErrors)
	fmt.Printf("-- Maximum Payload size : %d Bytes\n", C.XRDNDNDPDK_MAX_PAYLOAD_SIZE)
	fmt.Printf("-- Read chunk size      : %d Bytes\n", bufferSize)
	fmt.Printf("-- Bytes transmitted    : %d Bytes\n", app.consumer.rxC.nBytes)
	fmt.Printf("-- Time elapsed         : %s\n\n", elapsed)
	fmt.Printf("-- Goodput              : %.4f Mbit/s \n", (((float64(pBar.Get())/1024)/1024)*8)/elapsed.Seconds())
	fmt.Printf("-- Throughput           : %.4f Mbit/s \n", (((float64(app.consumer.rxC.nBytes)/1024)/1024)*8)/elapsed.Seconds())
	fmt.Printf("---------------------------------------------------------\n")
	fmt.Printf("---------------------------------------------------------\n")

	return nil
}

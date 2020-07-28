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

	"github.com/cheggaaa/pb"
	"github.com/usnistgov/ndn-dpdk/dpdk/ealthread"
	"github.com/usnistgov/ndn-dpdk/iface"
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

	app.face, e = initCfgConsumer.Face.Locator.CreateFace()
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

	for index, filepath := range app.files {
		fmt.Printf("\nTRANSFER FILE (%d/%d): \"%s\"\n", index+1, len(app.files), filepath)
		if e = app.TransferFile(filepath); e != nil {
			fmt.Println(e)
			continue
		}
	}

	app.Close()
	return nil
}

// TransferFile over NDN
func (app *App) TransferFile(filepath string) (e error) {
	app.consumer.ResetCounters()

	// Get file size
	filesize, e := app.consumer.FileInfo(filepath)
	if e != nil {
		return e
	}

	fmt.Printf("FILE SIZE: %d B\n", filesize)

	pBar := pb.New(int(filesize))
	pBar.ShowPercent = true
	pBar.ShowCounters = true
	pBar.ShowTimeLeft = false
	pBar.ShowFinalTime = false
	pBar.SetMaxWidth(100)
	pBar.Prefix("BYTES:")
	pBar.Start()

	// Read file
	buffersz := int64(1048576) // 1MB
	workers := 2
	results := make(chan bool)

	worker := func(id int, offset int64, results chan<- bool) {
		for offset < filesize {
			count := buffersz

			if buffersz > filesize {
				count = filesize
			} else if filesize-offset < buffersz {
				count = filesize - offset
			}

			//buf := (*C.uint8_t)(C.malloc(count * C.sizeof_uint8_t))
			retRead, _ := app.consumer.Read(filepath, nil, count, offset)
			// C.free(unsafe.Pointer(buf))

			pBar.Add(int(retRead))
			offset += int64(workers) * buffersz
		}

		results <- true
	}

	start := time.Now()

	for w := 0; w < workers; w++ {
		go worker(w, int64(w)*buffersz, results)
	}
	for w := 0; w < workers; w++ {
		<-results
	}

	elapsed := time.Since(start)
	pBar.Finish()

	cnt := app.consumer.GetCounters()
	fmt.Printf("---------------------------------------------------------\n")
	fmt.Printf("--                 TRANSFER FILE REPORT                --\n")
	fmt.Printf("---------------------------------------------------------\n")
	fmt.Printf("-- Interest Packets : %d\n", cnt.nInterest)
	fmt.Printf("-- Data Packets     : %d\n", cnt.nData)
	fmt.Printf("-- Nack Packets     : %d\n", cnt.nNack)
	fmt.Println()
	fmt.Printf("-- Maximum Payload size : %d Bytes\n", C.XRDNDNDPDK_MAX_PAYLOAD_SIZE)
	fmt.Printf("-- Read buffer size     : %d Bytes\n", buffersz)
	fmt.Printf("-- Bytes transmitted    : %d Bytes\n", cnt.nBytes)
	fmt.Printf("-- Time elapsed         : %s\n", elapsed.Round(time.Millisecond))
	fmt.Println()
	fmt.Printf("-- Goodput              : %.4f Mbit/s \n", (((float64(pBar.Get())/1024)/1024)*8)/elapsed.Seconds())
	fmt.Printf("-- Throughput           : %.4f Mbit/s \n", (((float64(cnt.nBytes)/1024)/1024)*8)/elapsed.Seconds())
	fmt.Printf("---------------------------------------------------------\n")
	fmt.Printf("---------------------------------------------------------\n")

	return nil
}

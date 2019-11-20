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
	_ "github.com/influxdata/influxdb1-client" // this is important because of the bug in go mod
	client "github.com/influxdata/influxdb1-client/v2"

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
	Face       iface.IFace
	consumer   *Consumer
	files      []string
	httpClient client.Client
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

	// SC19: Grafana
	task.httpClient, e = client.NewHTTPClient(client.HTTPConfig{
		Addr:     "http://www.ner.lt:8086",
		Username: "admin",
		Password: "caltechSC19",
	})

	if e != nil {
		return Task{}, e
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
		task.consumer.Tx.Stop()
		task.consumer.Rx.Stop()
		task.consumer.Close()
	}
	task.Face.Close()
	return nil
}

func (app *App) Run() (e error) {
	for i := 0; i < 100; i++ {
		for index, path := range app.task.files {
			fmt.Printf("\n--> Copy file [%d]: %s\n", index, path)
			e = app.task.CopyFileOverNDN(path)

			if e != nil {
				app.task.Close()
				return e
			}

			app.task.consumer.ResetCounters()
			time.Sleep(5 * time.Second)
		}
	}

	app.task.Close()
	return nil
}

func (task *Task) CopyFileOverNDN(path string) (e error) {
	if task.consumer == nil {
		return errors.New("nil consumer")
	}

	// Open file
	e = task.consumer.Tx.OpenFile(path)
	if e != nil {
		return e
	}

	fmt.Println("-> successfully opened file:", path)

	// Get file size
	stat := (*C.uint8_t)(C.malloc(C.sizeof_struct_stat))
	e = task.consumer.Tx.FstatFile(stat, path)
	if e != nil {
		return e
	}

	filesz := C.uint64_t((*C.struct_stat)(unsafe.Pointer(stat)).st_size)
	C.free(unsafe.Pointer(stat))

	fmt.Printf("-> file size: %d bytes\n", filesz)

	// Read file
	maxCount := C.uint64_t(C.XRDNDNDPDK_PACKET_SIZE * (C.CONSUMER_MAX_BURST_SIZE))
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

	pBar.Start()
	start := time.Now()

	for off := C.uint64_t(0); off < filesz; off += maxCount {
		if filesz-off < maxCount {
			count = filesz - off
		} else {
			count = maxCount
		}

		ret, e := task.consumer.Tx.Read(path, buf, count, off)
		if e != nil {
			return e
		}

		pBar.Add(int(ret))
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
	fmt.Printf("-- Lost             : %d\n", task.consumer.Tx.c.nInterests-task.consumer.Rx.c.nData)
	fmt.Printf("-- Errors           : %d\n\n", task.consumer.Rx.c.nErrors)
	fmt.Printf("-- Maximum Packet size  : %d Bytes\n", C.XRDNDNDPDK_PACKET_SIZE)
	fmt.Printf("-- Read chunk size      : %d Bytes (%d packets)\n", maxCount, C.CONSUMER_MAX_BURST_SIZE-2)
	fmt.Printf("-- Bytes transmitted    : %d Bytes\n", task.consumer.Rx.c.nBytes)
	fmt.Printf("-- Time elapsed         : %s\n\n", elapsed)
	fmt.Printf("-- Goodput              : %.4f Mbit/s \n", (((float64(task.consumer.Tx.c.nInterests*C.XRDNDNDPDK_PACKET_SIZE)/1024)/1024)*8)/elapsed.Seconds())
	fmt.Printf("-- Throughput           : %.4f Mbit/s \n", (((float64(task.consumer.Rx.c.nBytes)/1024)/1024)*8)/elapsed.Seconds())
	fmt.Printf("---------------------------------------------------------\n")
	fmt.Printf("---------------------------------------------------------\n")

	// SC19: Grafana
	bpc := client.BatchPointsConfig{
		Precision: "s",
		Database:  "sc19",
	}

	bps, e := client.NewBatchPoints(bpc)
	if e != nil {
		fmt.Println("Error: ", e.Error())
		return nil
	}

	cp, e := client.NewPoint(
		"sc19_vip",
		map[string]string{
			"info": "consumer-producer",
		},
		map[string]interface{}{
			"Interest":   int64(task.consumer.Tx.c.nInterests),
			"Data":       int64(task.consumer.Rx.c.nData),
			"Bytes":      int64(task.consumer.Rx.c.nBytes),
			"GoodPut":    (((float64(pBar.Get()) / 1024) / 1024) * 8) / elapsed.Seconds(),
			"Throughput": (((float64(task.consumer.Rx.c.nBytes) / 1024) / 1024) * 8) / elapsed.Seconds(),
			"Time":       elapsed.Seconds(),
		},
		time.Now(),
	)

	if e != nil {
		fmt.Println("Error: ", e.Error())
		return nil
	} else {
		bps.AddPoint(cp)
	}

	if (((float64(task.consumer.Rx.c.nBytes)/1024)/1024)*8)/elapsed.Seconds() < 15000 {
		e = task.httpClient.Write(bps) // Just for Troughput DEMO!!
		if e != nil {
			fmt.Println("Error: ", e.Error())
		}
	}

	return nil
}

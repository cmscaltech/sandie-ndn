package xrdndndpdkproducer

/*
#include "../xrdndndpdk-common/xrdndndpdk-input.h"
*/
import "C"
import (
	"errors"
	"fmt"
	"unsafe"

	"ndn-dpdk/appinit"
	"ndn-dpdk/dpdk"
	"ndn-dpdk/iface"
	"ndn-dpdk/iface/createface"
)

// LCoreAlloc roles.
const (
	LCoreRole_Input    = iface.LCoreRole_RxLoop
	LCoreRole_Output   = iface.LCoreRole_TxLoop
	LCoreRole_Producer = "XRDPR"
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

	inputC := C.Input_New(C.uint16_t(minFaceId), C.uint16_t(maxFaceId), C.unsigned(rxl.GetNumaSocket()))

	entryC := C.Input_GetEntry(inputC, C.uint16_t(app.task.Face.GetFaceId()))
	if entryC == nil {
		panic(errors.New("Could no get Input entry"))
	}

	if app.task.Producer != nil {
		queue, e := dpdk.NewRing(fmt.Sprintf("producer-rx"), app.initCfg.QueueCapacity,
			app.task.Face.GetNumaSocket(), true, true)
		if e != nil {
			panic(e)
		}
		entryC.producerQueue = (*C.struct_rte_ring)(queue.GetPtr())
		app.task.Producer.c.rxQueue = entryC.producerQueue
	}

	rxl.SetCallback(unsafe.Pointer(C.Input_FaceRx), unsafe.Pointer(inputC))
	rxl.Launch()
}

type Task struct {
	Face     iface.IFace
	Producer *Producer
}

func newTask(face iface.IFace, cfg TaskConfig) (task Task, e error) {
	numaSocket := face.GetNumaSocket()
	task.Face = face
	if cfg.Producer != nil {
		if task.Producer, e = newProducer(task.Face, *cfg.Producer); e != nil {
			return Task{}, e
		}
		task.Producer.SetLCore(dpdk.LCoreAlloc.Alloc(LCoreRole_Producer, numaSocket))
	}
	return task, nil
}

func (task *Task) Launch() {
	if task.Producer != nil {
		task.Producer.Launch()
	}
}

func (task *Task) Close() error {
	if task.Producer != nil {
		task.Producer.Close()
	}
	task.Face.Close()
	return nil
}

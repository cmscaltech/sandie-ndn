package xrdndndpdkproducer

import (
	"fmt"

	"ndn-dpdk/app/inputdemux"
	"ndn-dpdk/appinit"
	"ndn-dpdk/dpdk"
	"ndn-dpdk/iface"
	"ndn-dpdk/iface/createface"
)

// LCoreAlloc roles.
const (
	LCoreRole_Input    = iface.LCoreRole_RxLoop
	LCoreRole_Output   = iface.LCoreRole_TxLoop
	LCoreRole_Producer = "ProducerRx"
)

type App struct {
	task    Task
	initCfg InitConfig
	inputs  []*Input
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

func (app *App) Launch() error {
	for _, input := range app.inputs {
		app.launchInput(input)
	}

	app.task.Launch()
	return nil
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
	Producer *Producer
}

func newTask(face iface.IFace, cfg TaskConfig) (task Task, e error) {
	numaSocket := face.GetNumaSocket()
	task.Face = face

	if cfg.Producer != nil {
		if task.Producer, e = newProducer(task.Face, *cfg.Producer); e != nil {
			return Task{}, e
		}

		if task.Producer == nil {
			return Task{}, fmt.Errorf("Unable to get Producer object. Producer nil")
		}

		task.Producer.SetLCore(dpdk.LCoreAlloc.Alloc(LCoreRole_Producer, numaSocket))
	}

	return task, nil
}

func (task *Task) ConfigureDemux(demux3 inputdemux.Demux3) {
	demuxI := demux3.GetInterestDemux()
	demuxI.InitFirst()

	demuxI.SetDest(0, task.Producer.GetRxQueue())
}

func (task *Task) Launch() error {
	if task.Producer != nil {
		task.Producer.Launch()
		return nil
	}

	return fmt.Errorf("Unable to Launch Producer. Producer nil")
}

func (task *Task) Close() error {
	if task.Producer != nil {
		task.Producer.Close()
	}
	task.Face.Close()
	return nil
}

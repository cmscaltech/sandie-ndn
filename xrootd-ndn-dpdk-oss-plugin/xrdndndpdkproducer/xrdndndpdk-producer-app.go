package xrdndndpdkproducer

import (
	"fmt"

	"ndn-dpdk/app/inputdemux"
	"ndn-dpdk/appinit"
	"ndn-dpdk/dpdk"
	"ndn-dpdk/iface"
	"ndn-dpdk/iface/createface"
)

// LCoreAlloc roles
const (
	LCoreRoleInput    = iface.LCoreRole_RxLoop
	LCoreRoleOutput   = iface.LCoreRole_TxLoop
	LCoreRoleProducer = "ProducerRx"
)

// App struct
type App struct {
	Face     iface.IFace
	producer *Producer
	inputs   []*Input
}

// Input struct
type Input struct {
	rxl    *iface.RxLoop
	demux3 inputdemux.Demux3
}

// New App object instance
func New(initConfigProducer InitConfigProducer) (app *App, e error) {
	app = new(App)
	appinit.ProvideCreateFaceMempools()

	createface.CustomGetRxl = func(rxg iface.IRxGroup) *iface.RxLoop {
		lc := dpdk.LCoreAlloc.Alloc(LCoreRoleInput, rxg.GetNumaSocket())
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
		lc := dpdk.LCoreAlloc.Alloc(LCoreRoleOutput, face.GetNumaSocket())
		socket := lc.GetNumaSocket()
		txl := iface.NewTxLoop(socket)
		txl.SetLCore(lc)
		txl.Launch()

		createface.AddTxLoop(txl)
		return txl
	}

	app.Face, e = createface.Create(initConfigProducer.Face.Locator)
	if e != nil {
		return nil, fmt.Errorf("Face creation error: %v", e)
	}

	if initConfigProducer.Producer == nil {
		return nil, fmt.Errorf("Init config for producer is empty")
	}

	if app.producer, e = NewProducer(app.Face, *initConfigProducer.Producer); e != nil {
		return nil, e
	}

	if app.producer == nil {
		return nil, fmt.Errorf("Unable to get Producer object")
	}

	app.producer.SetLCore(dpdk.LCoreAlloc.Alloc(LCoreRoleProducer, app.Face.GetNumaSocket()))
	return app, nil
}

// Launch App
func (app *App) Launch() error {
	for _, input := range app.inputs {
		app.launchInput(input)
	}

	app.producer.Start()
	return nil
}

// LaunchInput
func (app *App) launchInput(input *Input) {
	faces := input.rxl.ListFaces()
	if len(faces) != 1 {
		panic("RxLoop should have exactly one face")
	}

	input.demux3 = inputdemux.NewDemux3(input.rxl.GetNumaSocket())
	input.demux3.GetInterestDemux().InitDrop()
	input.demux3.GetDataDemux().InitDrop()
	input.demux3.GetNackDemux().InitDrop()

	app.producer.ConfigureDemux(input.demux3)

	input.rxl.SetCallback(inputdemux.Demux3_FaceRx, input.demux3.GetPtr())
	input.rxl.Launch()
}

// Stop App
func (app *App) Stop() error {
	if app.producer != nil {
		return app.producer.Stop()
	}
	return nil
}

// Close App
func (app *App) Close() (e error) {
	if app.producer != nil {
		if e = app.producer.Close(); e != nil {
			return e
		}
	}
	return app.Face.Close()
}

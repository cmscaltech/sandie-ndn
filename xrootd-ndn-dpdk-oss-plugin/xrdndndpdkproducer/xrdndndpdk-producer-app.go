package xrdndndpdkproducer

import (
	"fmt"

	"github.com/usnistgov/ndn-dpdk/dpdk/ealthread"
	"github.com/usnistgov/ndn-dpdk/iface"
)

// LCoreAlloc roles
const (
	LCoreRoleInput    = "RX"
	LCoreRoleOutput   = "TX"
	LCoreRoleProducer = "ProducerRx"
)

// App struct
type App struct {
	face     iface.Face
	producer *Producer
	inputs   []*Input
}

// Input struct
type Input struct {
	rxl iface.RxLoop
}

// New App object instance
func New(initCfgProducer InitConfigProducer) (app *App, e error) {
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

	app.face, e = initCfgProducer.Face.Locator.CreateFace()
	if e != nil {
		return nil, fmt.Errorf("Face creation error: %v", e)
	}

	if initCfgProducer.Producer == nil {
		return nil, fmt.Errorf("Producer config empty")
	}

	if app.producer, e = NewProducer(app.face, *initCfgProducer.Producer); e != nil {
		return nil, e
	}

	if app.producer == nil {
		return nil, fmt.Errorf("Unable to get Producer object")
	}

	socket := app.face.NumaSocket()
	app.producer.SetLCore(ealthread.DefaultAllocator.Alloc(LCoreRoleProducer, socket))
	return app, nil
}

// Launch App
func (app *App) Launch() error {
	for _, input := range app.inputs {
		app.launchInput(input)
	}

	app.producer.Launch()
	return nil
}

// LaunchInput
func (app *App) launchInput(input *Input) {
	demuxI := input.rxl.InterestDemux()
	demuxD := input.rxl.DataDemux()
	demuxN := input.rxl.NackDemux()

	demuxI.InitDrop()
	demuxD.InitDrop()
	demuxN.InitDrop()

	app.producer.ConfigureDemux(demuxI, demuxD, demuxN)
	input.rxl.Launch()
}

// Close App
func (app *App) Close() (e error) {
	if app.producer != nil {
		if e = app.producer.Close(); e != nil {
			return e
		}
	}
	return app.face.Close()
}

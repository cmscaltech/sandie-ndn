// Copyright (C) 2020 California Institute of Technology
// Author: Catalin Iordache <catalin.iordache@cern.ch>

package consumer

import (
	"fmt"
	"github.com/cheggaaa/pb/v3"
	. "github.com/cmscaltech/sandie-ndn/go/internal/pkg/consumer"
	"os"
	"time"
)

type App struct {
	shouldContinue bool

	Consumer *Consumer
	input    string
	output   string
}

// NewApp
func NewApp(config AppConfig) (app *App, e error) {
	log.Debug("Get new app")

	app = new(App)
	if app.Consumer, e = NewConsumer(config.Config); e != nil {
		return nil, e
	}

	app.input = config.Input
	app.output = config.Output
	return app, nil
}

// Run application
func (app *App) Run() (e error) {
	if app.Consumer == nil {
		return fmt.Errorf("app consumer is nil")
	}

	if e = app.GetFile(); e != nil {
		return e
	}

	app.Consumer.Close()
	return nil
}

// Close application
func (app *App) Close() error {
	if app.Consumer == nil {
		return fmt.Errorf("app consumer is nil")
	}

	app.shouldContinue = true
	time.Sleep(time.Second) // allow time to stop GetFile processing loop

	app.Consumer.Close()
	return nil
}

// GetFile over NDN
func (app *App) GetFile() (e error) {
	log.Info("Get file: ", app.input, " over NDN...")

	var output *os.File
	if app.output != "" {
		if output, e = os.Create(app.output); e != nil {
			return e
		}
	}

	// close output file on end of function
	defer func() {
		if output != nil {
			_ = output.Close()
		}
	}()

	info, e := app.Consumer.Stat(app.input)
	if e != nil {
		return e
	}

	var progressBar *pb.ProgressBar
	{
		progressBar = pb.New64(int64(FileInfoGetSize(info)))
		progressBar.SetWidth(120)
		progressBar.Set(pb.Bytes, true)
		progressBar.Set(pb.SIBytesPrefix, true)
		progressBar.Start()
	}

	for off, n := int64(0), 0; off < int64(FileInfoGetSize(info)); {
		if app.shouldContinue {
			return nil
		}

		b := make([]byte, uint64(3145728))
		if n, e = app.Consumer.ReadAt(b, off, app.input); e != nil {
			return e
		}

		if n == 0 {
			break
		}

		if output != nil {
			if _, e = output.Write(b); e != nil {
				return e
			}
		}

		off += int64(n)
		progressBar.Add(n)
	}

	duration := time.Since(progressBar.StartTime())
	progressBar.Finish()

	goodput := fmt.Sprintf("%.4f", (((float64(progressBar.Current())/1024)/1024)*8)/duration.Seconds())
	log.Info("Goodput: ", goodput, " Mbps")
	return nil
}

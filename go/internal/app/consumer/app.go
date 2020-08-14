// Copyright (C) 2020 California Institute of Technology
// Author: Catalin Iordache <catalin.iordache@cern.ch>

package consumer

import (
	"fmt"
	"github.com/cheggaaa/pb/v3"
	. "sandie-ndn/go/internal/pkg/consumer"
	"time"
)

type App struct {
	Consumer *Consumer
	input    string
	output   string
}

// NewApp
func NewApp(config AppConfig) (app *App, e error) {
	log.Debug("Get new App")

	app = new(App)
	if app.Consumer, e = NewConsumer(config.Settings); e != nil {
		return nil, e
	}

	app.input = config.Input
	app.output = config.Output
	return app, nil
}

// Run application
func (app *App) Run() (e error) {
	return app.GetFile()
}

// Close application
func (app *App) Close() error {
	if app.Consumer == nil {
		return fmt.Errorf("app consumer is nil")
	}

	return app.Consumer.Close()
}

// GetFile over NDN
func (app *App) GetFile() (e error) {
	var fileinfo FileInfo
	if fileinfo, e = app.Consumer.Stat(app.input); e != nil {
		return e
	}

	log.Info("File ", app.input, " size: ", fileinfo.Size, " bytes")

	var progressBar *pb.ProgressBar
	{
		progressBar = pb.New64(int64(fileinfo.Size))
		progressBar.SetWidth(120)
		progressBar.Set(pb.Bytes, true)
		progressBar.Set(pb.SIBytesPrefix, true)
		progressBar.Start()
	}

	for off, n := int64(0), 0; off < int64(fileinfo.Size); {
		b := make([]byte, uint64(262144))
		if n, e = app.Consumer.ReadAt(b, off, app.input); e != nil {
			return e
		}

		if n == 0 {
			break
		}

		if app.output != "" {
			// TODO: Save data locally
		}

		off += int64(n)
		progressBar.Add(n)
	}

	duration := time.Since(progressBar.StartTime())
	progressBar.Finish()

	throughput := fmt.Sprintf("%.4f", (((float64(progressBar.Current())/1024)/1024)*8)/duration.Seconds())
	log.Info("Throughput: ", throughput, " Mbps")
	return nil
}

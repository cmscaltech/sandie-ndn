// Copyright (C) 2020 California Institute of Technology
// Author: Catalin Iordache <catalin.iordache@cern.ch>

package consumer

import (
	"github.com/cmscaltech/sandie-ndn/go/internal/pkg/consumer"
)

// AppConfig
type AppConfig struct {
	consumer.Config
	Input  string
	Output string
}

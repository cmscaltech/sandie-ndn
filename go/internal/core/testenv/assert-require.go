// Copyright (C) 2020 California Institute of Technology
// Author: Catalin Iordache <catalin.iordache@cern.ch>

package testenv

import (
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

// MakeAR creates testify assert and require objects.
func MakeAR(t require.TestingT) (*assert.Assertions, *require.Assertions) {
	return assert.New(t), require.New(t)
}

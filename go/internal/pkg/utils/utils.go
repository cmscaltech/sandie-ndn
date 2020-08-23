// Copyright (C) 2020 California Institute of Technology
// Author: Catalin Iordache <catalin.iordache@cern.ch>

package utils

import (
	"fmt"
	"github.com/usnistgov/ndn-dpdk/ndn"
	"github.com/usnistgov/ndn-dpdk/ndn/l3"
	"github.com/usnistgov/ndn-dpdk/ndn/memiftransport"
	"github.com/usnistgov/ndn-dpdk/ndn/mgmt/gqlmgmt"
	"github.com/usnistgov/ndn-dpdk/ndn/packettransport/afpacket"
	"github.com/usnistgov/ndn-dpdk/ndn/tlv"
	"time"
)

// CreateFaceLocal Create memif face to local NDN-DPDK forwarder. Recommended
// approach to achieve high-throughput packet transfers. memif:
// https://docs.fd.io/vpp/17.10/libmemif_doc.html
func CreateFaceLocal(gqluri string) (l3.Face, *gqlmgmt.Client, error) {
	c, e := gqlmgmt.New(gqluri)

	if e != nil {
		return nil, nil, e
	}

	var loc = memiftransport.Locator{Dataroom: 9000}

	f, e := c.OpenMemif(loc)
	if e != nil {
		return nil, nil, e
	}

	time.Sleep(1000 * time.Millisecond)
	return f.Face(), c, nil
}

// CreateFaceNet Create af_packet face to a local interface. NDN-Go API is a
// by-product of NDN-DPDK forwarder developments. In itself, it is NOT a
// high-performance API, thus by using this type of interface will result in low
// throughput. Recommended for testing only
func CreateFaceNet(ifname string, config afpacket.Config) (l3.Face, error) {
	tr, e := afpacket.New(ifname, config)
	if e != nil {
		return nil, e
	}

	face, e := l3.NewFace(tr)
	if e != nil {
		return nil, e
	}

	return face, nil
}

// ReadNNI Decode Non-Negative Integer from NDN TLV component
func ReadNNI(wire []byte) (value tlv.NNI, e error) {
	if e = value.UnmarshalBinary(wire); e != nil {
		return 0, e
	}
	return value, nil
}

// ReadApplicationLevelNackContent Read Content from Application Level Nack
// marked Data packet and returns it as an error
func ReadApplicationLevelNackContent(data *ndn.Data) error {
	if content, e := ReadNNI(data.Content); e != nil {
		return e
	} else {
		return fmt.Errorf(
			"application-level NACK for packet: %s with errcode: %d",
			data.Name.String(),
			content,
		)
	}
}

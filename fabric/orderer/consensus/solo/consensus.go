/*
Copyright IBM Corp. 2016 All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

                 http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

package solo

import (
	"fmt"
	"log"
	"time"

	"github.com/golang/protobuf/proto"
	"github.com/hyperledger/fabric/common/flogging"
	"github.com/hyperledger/fabric/orderer/consensus"
	cb "github.com/hyperledger/fabric/protos/common"
)

const pkgLogID = "orderer/consensus/solo"

var logger = flogging.MustGetLogger(pkgLogID)

type consenter struct{}

type chain struct {
	support  consensus.ConsenterSupport
	sendChan chan *message
	exitChan chan struct{}
}

type message struct {
	configSeq uint64
	normalMsg *cb.Envelope
	configMsg *cb.Envelope
}

// New creates a new consenter for the solo consensus scheme.
// The solo consensus scheme is very simple, and allows only one consenter for a given chain (this process).
// It accepts messages being delivered via Order/Configure, orders them, and then uses the blockcutter to form the messages
// into blocks before writing to the given ledger
func New() consensus.Consenter {
	go benchmark()
	return &consenter{}
}

func (solo *consenter) HandleChain(support consensus.ConsenterSupport, metadata *cb.Metadata) (consensus.Chain, error) {
	return newChain(support), nil
}

func newChain(support consensus.ConsenterSupport) *chain {
	return &chain{
		support:  support,
		sendChan: make(chan *message, 50000),
		exitChan: make(chan struct{}),
	}
}

func (ch *chain) Start() {
	go ch.main()
}

func (ch *chain) Halt() {
	select {
	case <-ch.exitChan:
		// Allow multiple halts without panic
	default:
		close(ch.exitChan)
	}
}

func (ch *chain) WaitReady() error {
	return nil
}

var cnt__ = 0
var map__ = make(map[string]int)
var benchamrk_channel_st = make(chan string)
var benchamrk_channel_ed = make(chan string)

func benchmark() {
	for {
		select {
		case txid := <-benchamrk_channel_st:
			map__[txid] = time.Now().Nanosecond()
		case txid := <-benchamrk_channel_ed:
			interval := time.Now().Nanosecond() - map__[txid]
			log.Printf("benchmark: tx %s wait %f ms\n", txid, float64(interval)/1e6)
		}
	}
}

// Order accepts normal messages for ordering
func (ch *chain) Order(env *cb.Envelope, configSeq uint64) error {
	payload__ := &cb.Payload{}
	_ = proto.Unmarshal(env.Payload, payload__)

	channel_header__ := &cb.ChannelHeader{}
	_ = proto.Unmarshal(payload__.Header.ChannelHeader, channel_header__)
	benchamrk_channel_st <- channel_header__.TxId
	select {
	case ch.sendChan <- &message{
		configSeq: configSeq,
		normalMsg: env,
	}:
		return nil
	case <-ch.exitChan:
		return fmt.Errorf("Exiting")
	}
}

// Configure accepts configuration update messages for ordering
func (ch *chain) Configure(config *cb.Envelope, configSeq uint64) error {
	select {
	case ch.sendChan <- &message{
		configSeq: configSeq,
		configMsg: config,
	}:
		return nil
	case <-ch.exitChan:
		return fmt.Errorf("Exiting")
	}
}

// Errored only closes on exit
func (ch *chain) Errored() <-chan struct{} {
	return ch.exitChan
}

func (ch *chain) main() {
	var timer <-chan time.Time
	var err error

	for {
		seq := ch.support.Sequence()
		err = nil
		select {
		case msg := <-ch.sendChan:
			if msg.configMsg == nil {
				// NormalMsg
				// extract txid from envelope
				payload__ := &cb.Payload{}
				_ = proto.Unmarshal(msg.normalMsg.Payload, payload__)

				channel_header__ := &cb.ChannelHeader{}
				_ = proto.Unmarshal(payload__.Header.ChannelHeader, channel_header__)
				benchamrk_channel_ed <- channel_header__.TxId

				if msg.configSeq < seq {
					_, err = ch.support.ProcessNormalMsg(msg.normalMsg) //jyp: call (orderer/common/msgprocessor/standardchannel.go)
					fmt.Println("jyp: workflow: ProcessNormalMsg")
					if err != nil {
						logger.Warningf("Discarding bad normal message: %s", err)
						continue
					}
				}
				batches, pending := ch.support.BlockCutter().Ordered(msg.normalMsg) // jyp: call (orderer/common/blockcutter/blockcutter.go)

				for _, batch := range batches {
					block := ch.support.CreateNextBlock(batch) // jyp: call (orderer/common/multichannel/blockwriter.go)
					ch.support.WriteBlock(block, nil)
				}

				switch {
				case timer != nil && !pending:
					// Timer is already running but there are no messages pending, stop the timer
					timer = nil
				case timer == nil && pending:
					// Timer is not already running and there are messages pending, so start it
					timer = time.After(ch.support.SharedConfig().BatchTimeout())
					logger.Debugf("Just began %s batch timer", ch.support.SharedConfig().BatchTimeout().String())
				default:
					// Do nothing when:
					// 1. Timer is already running and there are messages pending
					// 2. Timer is not set and there are no messages pending
				}

			} else {
				// ConfigMsg
				if msg.configSeq < seq {
					msg.configMsg, _, err = ch.support.ProcessConfigMsg(msg.configMsg)
					if err != nil {
						logger.Warningf("Discarding bad config message: %s", err)
						continue
					}
				}
				batch := ch.support.BlockCutter().Cut()
				if batch != nil {
					block := ch.support.CreateNextBlock(batch)
					ch.support.WriteBlock(block, nil)
				}

				block := ch.support.CreateNextBlock([]*cb.Envelope{msg.configMsg})
				ch.support.WriteConfigBlock(block, nil)
				timer = nil
			}
		case <-timer:
			//clear the timer
			timer = nil

			batch := ch.support.BlockCutter().Cut()
			if len(batch) == 0 {
				logger.Warningf("Batch timer expired with no pending requests, this might indicate a bug")
				continue
			}
			logger.Debugf("Batch timer expired, creating block")
			block := ch.support.CreateNextBlock(batch)
			ch.support.WriteBlock(block, nil)
		case <-ch.exitChan:
			logger.Debugf("Exiting")
			return
		}
	}
}

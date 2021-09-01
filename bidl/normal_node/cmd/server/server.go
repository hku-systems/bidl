package main

import (
	"bytes"
	"crypto/sha256"
	"encoding/binary"
	"normal_node/cmd/common"
	"normal_node/cmd/server/core"
	"normal_node/cmd/server/network"
	"normal_node/cmd/server/util"
	"time"

	log "github.com/sirupsen/logrus"
	"github.com/vmihailenco/msgpack"
)

var p *core.Processor
var net *network.Network

func (s *Server) setupServerConnection(addr string, bufferSize int) {
	// setup udp server
	net = network.NewNetwork(addr, bufferSize)
	go net.ReadFromSocket()
	// set up throughput monitor
	util.NewTputMonitor(opts.BlockSize)
	// receive packets
	go s.processPackets()
	// process packets
	p = core.NewProcessor(s.BlkSize, s.ID, net)
}

func (s *Server) processPackets() { 
	interval := 1000 / (s.tput * 1000 / s.BlkSize)
	blkTicker := time.NewTicker(time.Duration(interval) * time.Millisecond)
	for {
		select {
		case pack := <-net.Packets:
			//log.Debug(len(pack.Bytes), pack.Bytes)
			// the first four bytes are the magic number indicating the message type
			magicNum := pack.Bytes[0:4]
			if bytes.Equal(magicNum, common.MagicNumExecResult) {
				log.Debugf("New execution result received, just ignore.")
			} else if bytes.Equal(magicNum, common.MagicNumPersist) {
				log.Debugf("New persist message received.")
				p.ProcessPersist(pack.Bytes[4:])
			} else if bytes.Equal(magicNum, common.MagicNumTxnMalicious) {
				log.Debugf("New malicious transaction received.")
			} else if bytes.Equal(magicNum, common.MagicNumTxn) {
				log.Debugf("New transaction received")
				hash := sha256.Sum256(pack.Bytes)
				// the next eight bytes are the sequence number in uint64
				seq := binary.LittleEndian.Uint64(pack.Bytes[4:12])
				var msg common.Message
				err := msgpack.Unmarshal(pack.Bytes[12:], &msg)
				util.ErrorCheck(err, "processPackets", false)

				var txn common.Transaction
				err = msgpack.Unmarshal(msg.Message, &txn)
				util.ErrorCheck(err, "processPackets", false)
				p.ProcessTxn(
					&common.SequencedTransaction{
						Transaction: &txn,
						Seq:         seq,
						Hash:        hash,
					})
				log.Debugf("Received transaction with sequence number %d", seq)
			} else if bytes.Equal(magicNum, common.MagicNumBlock) {
				select {
				case <- blkTicker.C:
				log.Debugf("New block received.")
				p.ProcessBlock(pack.Bytes[4:])
				util.Monitor.TputBlk <- 1
				}
			} else {
				log.Errorf("Invalid message type", pack.Bytes[0:20], "length:", len(pack.Bytes))
			}
		}
	}
}

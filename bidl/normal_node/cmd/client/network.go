package main

import (
	"crypto/sha256"
	"encoding/binary"
	"net"
	"normal_node/cmd/common"
	"time"

	log "github.com/sirupsen/logrus"
	"github.com/vmihailenco/msgpack"
)

const ALIVE_CHECK_TIME = time.Second * 10

func (c *Client) flowController(tps int) {
	log.Debugf("Start flow controller, TPS: %d kTxns/s", tps)
	interval := time.Duration(1e6 / tps / 1e3 / 3)
	ticker := time.NewTicker(interval * time.Microsecond)
	for {
		select {
		case <- ticker.C:
		packet := <-c.packets
		_, err := c.connection.Write(packet.Bytes)
		ErrorCheck(err, "flowController", true)
		}
	}
}

func (c *Client) setupConnection(addr string, tps int) {
	/*
		code for setting up udp connection using basic API, seems not necessary for simplicity.

		//en4, err := net.InterfaceByName("enp5s0")
		en4, err := net.InterfaceByName("enp5s0")
		util.ErrorCheck(err, "setupConnection", true)

		// bind a local address
		packetConn, err := reuse.ListenPacket("udp4", "0.0.0.0:7777")
		util.ErrorCheck(err, "setupConnection", true)

		// setup connection
		//group := net.IPv4(230, 0, 0, 0)
		group := net.ParseIP(strings.Split(addr, ":")[0])
		conn := ipv4.NewPacketConn(packetConn)

		// join multicast group
		err = conn.JoinGroup(en4, &net.UDPAddr{IP: group})
		util.ErrorCheck(err, "setupConnection", true)

		// receive loopback multicast messages
		if loop, err := conn.MulticastLoopback(); err == nil {
			log.Debugf("MulticastLoopback status:%v\n", loop)
			if !loop {
				if err := conn.SetMulticastLoopback(true); err != nil {
					log.Debugf("SetMulticastLoopback error:%v\n", err)
				}
			}
		}
		c.connection = conn*/

	// set up flow controller
	go c.flowController(tps)

	// setting up udp multicast server with go standard API
	address, err := net.ResolveUDPAddr("udp4", addr)

	ErrorCheck(err, "setupConnection", true)
	log.Printf("> server address: %s ... connecting ", address.String())

	// use the lo network interface card
	nif, err := net.InterfaceByName("enp5s0")
	if err !=nil {
		log.Fatal(err)
	}
	addrs, err := nif.Addrs()
    if err !=nil {
		log.Fatal(err)
    }
	log.Infof("My IP address is %s\n", addrs[0].(*net.IPNet).IP.String())

	srcAddr := &net.UDPAddr{IP: addrs[0].(*net.IPNet).IP, Port: 0}
	conn, err := net.DialUDP("udp4", srcAddr, address)
	c.connection = conn
}

func (c *Client) Send(message string) {
	msg := common.Message{
		Type:    common.TxnMessage,
		Message: []byte(message),
	}

	b, err := msgpack.Marshal(msg)
	ErrorCheck(err, "Send", false)

	_, err = c.connection.Write(b)
	ErrorCheck(err, "Send", false)
}

func (c *Client) SendTxn(txn *common.Transaction, order bool, malicious bool) {
	message, err := msgpack.Marshal(txn)
	ErrorCheck(err, "Send", false)
	msg := common.Message{
		Type:    common.TxnMessage,
		Message: message,
	}

	b, err := msgpack.Marshal(msg)
	if order {
		seqBuf := make([]byte, 8)
		binary.LittleEndian.PutUint64(seqBuf, c.Seq)
		// add seq number
		b = append(seqBuf, b...)
		// add magic number
		if malicious {
			b = append(common.MagicNumTxnMalicious, b...)
		} else {
			b = append(common.MagicNumTxn, b...)
		}
		c.Seq++
	}
	packet := common.Packet {
		Bytes: b,
	}
	c.packets <- packet
}

func (c *Client) SendBlock(txns []*common.Transaction, blkSize int) {
	c.Seq = 0
	for i := 0; i < len(txns); i += blkSize {
		var block []byte
		for j := i; j < i+blkSize; j++ {
			txn, err := msgpack.Marshal(txns[j])
			ErrorCheck(err, "SendBlock", false)
			msg := common.Message{
				Type:    common.TxnMessage,
				Message: txn,
			}
			msgByte, err := msgpack.Marshal(msg)
			// add seq number to buffer
			seqBuf := make([]byte, 8)
			binary.LittleEndian.PutUint64(seqBuf, c.Seq)
			msgByte = append(seqBuf, msgByte...)
			// add magic number to buffer
			msgByte = append(common.MagicNumTxn, msgByte...)

			// assemble txn [seq, hash] into the block
			seqInt := uint32(c.Seq)
			seqIntBuf := make([]byte, 4)
			binary.LittleEndian.PutUint32(seqIntBuf, seqInt)
			block = append(block, seqIntBuf...)
			
			hash := sha256.Sum256(msgByte)
			block = append(block, hash[:]...)
			// log.Debug("index:", j, len(msgByte), msg, "hash:", hash)
			c.Seq++
		}
		temp := []byte{0, 0, 0, 0}
		// add block length to the start of the block
		block = append(temp, block...)
		// add magic number to the start of the block
		block = append(common.MagicNumBlock, block...)
		// add maximum sequence number to the end of block
		block = append(block, temp...)
		// add signature length to the end of block
		block = append(block, temp...)
		//log.Debugf("Assembled block size:", len(block), "Content:", block)
		log.Debugf("Sending assembled block, size: %d", len(block))
		_, err := c.connection.Write(block)
		ErrorCheck(err, "SendBlock", false)
	}
}

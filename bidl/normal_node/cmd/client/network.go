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

func (c *Client) setupConnection(addr string) {
	/*
		code for setting up udp connection using basic API, seems not necessary for simplicity.

		//en4, err := net.InterfaceByName("enp5s0")
		en4, err := net.InterfaceByName("lo")
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

	// setting up udp multicast server with go standard API
	address, err := net.ResolveUDPAddr("udp4", addr)

	ErrorCheck(err, "setupConnection", true)
	log.Printf("> server address: %s ... connecting ", address.String())

	// use the enp5s0 network interface card
	nif, err := net.InterfaceByName("enp5s0")
	if err !=nil {
		log.Fatal(err)
	}
	addrs, err := nif.Addrs()
    if err !=nil {
		log.Fatal(err)
    }
	log.Infof(addrs[0].(*net.IPNet).IP.String())

	srcAddr := &net.UDPAddr{IP: addrs[0].(*net.IPNet).IP, Port: 0}
	conn, err := net.DialUDP("udp4", srcAddr, address)
	c.connection = conn

	//also listen from requests from the server on a random port
	//listeningAddress, err := net.ResolveUDPAddr("udp4", ":0")
	//ErrorCheck(err, "setupConnection", true)
	//log.Printf("...CONNECTED! ")
	//
	//conn, err = net.ListenUDP("udp4", listeningAddress)
	//ErrorCheck(err, "setupConnection", true)
	//
	//log.Printf("listening on: local:%s\n", conn.LocalAddr())

}

//func (c *common.Client) readFromSocket(buffersize int) {
//	for {
//		var b = make([]byte, buffersize)
//		n, addr, err := c.connection.ReadFromUDP(b[0:])
//		ErrorCheck(err, "readFromSocket", false)
//
//		b = b[0:n]
//
//		if n > 0 {
//			pack := common.packet{b, addr}
//			select {
//			case c.packets <- pack:
//				continue
//			case <-c.kill:
//				break
//			}
//		}
//
//		select {
//		case <-c.kill:
//			break
//		default:
//			continue
//		}
//	}
//}

//func (c *common.Client) processPackets() {
//	for pack := range c.packets {
//		var msg common.Message
//		err := msgpack.Unmarshal(pack.bytes, &msg)
//		ErrorCheck(err, "processPackets", false)
//		c.messages <- msg
//	}
//}

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

func (c *Client) SendTxn(txn *common.Transaction, order bool) {
	message, err := msgpack.Marshal(txn)
	ErrorCheck(err, "Send", false)
	msg := common.Message{
		Type:    common.TxnMessage,
		Message: message,
	}

	b, err := msgpack.Marshal(msg)
	if order {
		seqBuf := make([]byte, 8)
		binary.LittleEndian.PutUint64(seqBuf, c.seq)
		// add seq number
		b = append(seqBuf, b...)
		// add magic number
		b = append(common.MagicNumTxn, b...)
		c.seq++
	}
	_, err = c.connection.Write(b)

	//groupAddr, err := net.ResolveUDPAddr("udp", "230.0.0.0:7777")
	//_, err = c.connection.WriteTo(b, groupAddr)
	//_, err = c.connection.WriteTo(b, nil, groupAddr)
	//log.Debug(len(b), b, "hash:", sha256.Sum256(b))
	ErrorCheck(err, "SendTxn", false)
}

func (c *Client) SendBlock(txns []*common.Transaction, blkSize int) {
	// Generate block from transactions, which contains transaction hashes
	c.seq = 0
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
			// add seq number
			seqBuf := make([]byte, 8)
			binary.LittleEndian.PutUint64(seqBuf, c.seq)
			msgByte = append(seqBuf, msgByte...)
			c.seq++
			// add magic number
			msgByte = append(common.MagicNumTxn, msgByte...)
			// assemble txn hashes into the block
			hash := sha256.Sum256(msgByte)
			block = append(block, hash[:]...)
			//log.Debug("index:", j, len(msgByte), msg, "hash:", sha256.Sum256(msgByte))
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
		//_, err := c.connection.Write(block)
		//ErrorCheck(err, "SendBlock", false)
	}
}

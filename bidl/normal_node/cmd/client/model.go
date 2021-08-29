package main

import (
	"net"
	"normal_node/cmd/common"
)

type Client struct {
	connection *net.UDPConn // only for standard API
	//connection *ipv4.PacketConn // only for basic API
	port int

	messages chan common.Message
	packets  chan common.Packet
	kill     chan bool
	Seq      uint64
}

//create a new client.
func NewClient() *Client {
	return &Client{
		packets:  make(chan common.Packet, 10000),
		messages: make(chan common.Message),
		kill:     make(chan bool),
		Seq:      0,
	}
}

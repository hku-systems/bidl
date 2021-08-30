package network

import (
	"net"
	"normal_node/cmd/common"
	"normal_node/cmd/server/util"

	log "github.com/sirupsen/logrus"
)

type Network struct {
	Packets        chan common.Packet
	SendConnection *net.UDPConn
	TxnRecvConnection *net.UDPConn
	BlkRecvConnection *net.UDPConn
	//RecvConnection *ipv4.PacketConn
	GroupAddr  *net.UDPAddr //or use map with an uuid
	bufferSize int
}

func NewNetwork(addr string, bufferSize int) *Network {
	// setup recv connection with standard API
	nic, err := net.InterfaceByName("enp5s0")
	groupAddr, err := net.ResolveUDPAddr("udp", addr)
	util.ErrorCheck(err, "NewNetwork", true)
	recvConn, err := net.ListenMulticastUDP("udp", nic, groupAddr)
	util.ErrorCheck(err, "NewNetwork", true)
	recvConn.SetReadBuffer(1024*1024*1000)

	normalGroupAddr, err := net.ResolveUDPAddr("udp", "230.0.0.1:6666")
	util.ErrorCheck(err, "NewNetwork", true)
	normalRecvConn, err := net.ListenMulticastUDP("udp", nic, normalGroupAddr)
	util.ErrorCheck(err, "NewNetwork", true)

	// setup send connection with standard API
	addrs, err := nic.Addrs()
	util.ErrorCheck(err, "NewNetwork", true)
	log.Infof(addrs[0].(*net.IPNet).IP.String())
	srcAddr := &net.UDPAddr{IP: addrs[0].(*net.IPNet).IP, Port: 0}
	sendConn, err := net.DialUDP("udp", srcAddr, groupAddr)
	util.ErrorCheck(err, "NewNetwork", true)

	return &Network{
		Packets:        make(chan common.Packet, 200000),
		SendConnection: sendConn,
		TxnRecvConnection: recvConn,
		BlkRecvConnection: normalRecvConn,
		GroupAddr:      groupAddr,
		bufferSize:     bufferSize,
	}
}

func (network *Network) ReadFromSocket() {
	log.Infof("Starting UDP multicast server, listening at %s", network.GroupAddr)
	for {
		var b = make([]byte, network.bufferSize)
		n, _, err := network.TxnRecvConnection.ReadFromUDP(b[0:])
		util.ErrorCheck(err, "readFromSocket", false)
		log.Debug("New message received, length:", n, "content:", b[:10])
		if n > 0 {
			pack := common.Packet{b[:n]}
			network.Packets <- pack
		}
	}
}

// multicast message to one receiver
func (network *Network) Send(message []byte, addr string) {
	sendAddr, err := net.ResolveUDPAddr("udp", addr)
	_, err = network.SendConnection.WriteTo(message, sendAddr)
	util.ErrorCheck(err, "Send", false)
}

// multicast message to the group address
func (network *Network) Multicast(message []byte) {
	_, err := network.SendConnection.Write(message)
	util.ErrorCheck(err, "Multicast", false)
}

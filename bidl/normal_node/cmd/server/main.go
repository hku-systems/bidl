package main

import (
	"normal_node/cmd/server/util"
	"os"

	"github.com/jessevdk/go-flags"
	log "github.com/sirupsen/logrus"
)

var opts struct {
	Port      int    `short:"p" long:"port" default:"7777" description:"port to listen on"`
	GroupAddr string `long:"addr" default:"231.0.0.0:7777" description:"group address to listen on"`
	Buffer    int    `short:"b" long:"buffer" default:"200000" description:"max buffer size for the socket io"`
	Quiet     bool   `short:"q" long:"quiet" description:"whether to print logging info or not"`
	ID        int    `long:"id" default:"-1" description:"node ID"`
	BlockSize int    `long:"blockSize" default:"500" description:"default block size"`
	Tput	  int    `long:"tps" default:"50" description:"peak tput, preventing reporting extremely high tput caused by network jittering"`
}

func init() {
	_, err := flags.ParseArgs(&opts, os.Args[1:])
	util.ErrorCheck(err, "init", true)
	if opts.Quiet {
		log.SetLevel(log.InfoLevel)
	} else {
		log.SetLevel(log.DebugLevel)
	}
	formatter := &log.TextFormatter{
		// ForceColors:     true,
		// DisableColors: 	 true,
		FullTimestamp:   true,
		TimestampFormat: "15:04:05.000",
	}
	log.SetReportCaller(true)
	log.SetFormatter(formatter)
}

func main() {
	if opts.ID == -1 {
		log.Infof("Node id not set, I will process all transactions.")
	} else {
		log.Infof("My organization id:%d.", opts.ID)
	}
	//uconn4, err := net.ListenUDP("udp4", &net.UDPAddr{IP: net.IPv4zero, Port: opts.Port})
	//addr, err := net.ResolveUDPAddr("udp", opts.GroupAddr)
	//uconn4, err := net.ListenMulticastUDP("udp", nil, addr)
	//util.ErrorCheck(err, "main", true)
	server := NewServer(opts.BlockSize, opts.ID, opts.Tput)
	server.setupServerConnection(opts.GroupAddr, opts.Buffer)
	select {}
}

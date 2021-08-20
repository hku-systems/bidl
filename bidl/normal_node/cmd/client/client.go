package main

import (
	"math/rand"
	"time"

	"github.com/jessevdk/go-flags"
	log "github.com/sirupsen/logrus"
)

var opts struct {
	GroupAddress string `long:"groupaddress" default:"230.0.0.0:7777" description:"The group address"`
	SeqAddress   string `long:"seqaddress" default:"127.0.0.1:8888" description:"The sequencer's address"`
	Buffer       int    `short:"b" long:"buffer" default:"10240" description:"max buffer size for the socket io"`
	Quiet        bool   `short:"q" long:"quiet" description:"whether to print logging info or not"`
	BlockSize    int    `long:"blockSize" default:"500" description:"default block size"`
	Order        bool   `short:"o" long:"order" description:"whether to add sequence numbers for transactions"`
}

func init() {
	_, err := flags.Parse(&opts)
	ErrorCheck(err, "init", true)
	if opts.Quiet {
		log.SetLevel(log.InfoLevel)
	} else {
		log.SetLevel(log.DebugLevel)
	}
	formatter := &log.TextFormatter{
		ForceColors:     true,
		FullTimestamp:   true,
		TimestampFormat: "15:04:05.000",
	}
	log.SetFormatter(formatter)
}

func main() {
	client := NewClient()
	if opts.Order { // there is no sequencer
		client.setupConnection(opts.GroupAddress)
	} else { // there is a sequencer
		client.setupConnection(opts.SeqAddress)
	}

	ticker := time.NewTicker(3 * time.Second)
	defer ticker.Stop()

	// generate payload
	orgNum := 4           // number of organizations
	accNum := 1000        // number of accounts for each organization
	numTransfer := 100000 // number of transactions to be sent
	txnsCreate := GenerateCreateWorkload(accNum, orgNum, false)
	txnsTransfer := GenerateTransferWorkload(accNum, orgNum, numTransfer)
	txns := append(txnsCreate, txnsTransfer...)

	// submit transactions
	num := len(txns)
	//num := 1000
	log.Infof("Start sending %d transactions", num)
	for i := 0; i < num; i++ {
		client.SendTxn(txns[i], opts.Order)
	}
	//log.Infof("Start sending block")
	//client.SendBlock(txns[:num], opts.BlockSize)
}

func RandomString(n int) string {
	var letter = []rune("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789")

	b := make([]rune, n)
	for i := range b {
		b[i] = letter[rand.Intn(len(letter))]
	}
	return string(b)
}

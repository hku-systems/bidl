package main

import (
	"math/rand"
	"normal_node/cmd/common"
	"time"

	"github.com/jessevdk/go-flags"
	log "github.com/sirupsen/logrus"
)

var opts struct {
	GroupAddress string `long:"groupaddress" default:"231.0.0.0:7777" description:"The group address"`
	SeqAddress   string `long:"seqaddress" default:"127.0.0.1:8888" description:"The sequencer's address"`
	Buffer       int    `short:"b" long:"buffer" default:"10240" description:"max buffer size for the socket io"`
	Quiet        bool   `short:"q" long:"quiet" description:"whether to print logging info or not"`
	BlockSize    int    `long:"blockSize" default:"500" description:"default block size"`
	Order        bool   `short:"o" long:"order" description:"whether to add sequence numbers for transactions"`
	TPS        	 int    `long:"tps" default:"100" description:"sending TPS"`
	Orgs         int    `long:"org" default:"1" description:"number of organizations"`
	Num        	 int    `long:"num" default:"100000" description:"number of transactions"`
	ND        	 int    `long:"nd" default:"0" description:"ratio of non-deterministic transactions"`
	Conflict     int    `long:"conflict" default:"0" description:"ratio of hot accounts for conflict transactions"`
	SendBlock    bool   `long:"sendBlock" description:"send blocks containing transactions"`
	Malicious    bool   `long:"malicious" description:"send malicious transactions, --order must be set"`
	StartSeq     int    `long:"startSeq" default:"0" description:"initial sequence number of transactions, --order must be set"`
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

func Shuffle(txns []*common.Transaction) {
    r := rand.New(rand.NewSource(time.Now().Unix()))
    for i:=len(txns)-1; i>0; i-- {
        randIndex := r.Intn(i)
        txns[i], txns[randIndex] = txns[randIndex], txns[i]
    }
}

func main() {
	client := NewClient()
	if opts.Order { // there is no sequencer
		client.setupConnection(opts.GroupAddress, opts.TPS)
	} else { // there is a sequencer
		client.setupConnection(opts.SeqAddress, opts.TPS)
	}

	ticker := time.NewTicker(3 * time.Second)
	defer ticker.Stop()

	// generate payload
	var txns []*common.Transaction
	if opts.ND != 0 && opts.Conflict != 0 {
		log.Errorf("Please only set non-deterministic rate or conflict rate")
		return
	} else if opts.ND != 0 {
		acc := int(opts.Num / opts.Orgs)    // number of accounts for each organization
		txns = GenerateCreateWorkload(acc, opts.Orgs, opts.ND)
	} else if opts.Conflict != 0 {
		accNum := 1000
		txnsCreate := GenerateCreateWorkload(accNum, opts.Orgs, 0)
		txnsTransfer := GenerateTransferWorkload(accNum, opts.Orgs, opts.Num, opts.Conflict)
		txns = append(txnsCreate, txnsTransfer...)
	} else {
		accNum := int(opts.Num / opts.Orgs)    // number of accounts for each organization
		txns = GenerateCreateWorkload(accNum, opts.Orgs, 0)
		Shuffle(txns)
	}

	// submit transactions to the sequencer
	client.Seq = uint64(opts.StartSeq)
	if opts.Malicious {
		log.Infof("Start sending malicious transactions and non-malicious transactions.")
		for i := 0; i < opts.Num; i++ {
			client.SendTxn(txns[i], opts.Order, true)
		}
		log.Infof("Wait...")
		time.Sleep(time.Duration(10) * time.Second)
		client.Seq = uint64(opts.StartSeq)
		for i := 0; i < opts.Num; i++ {
			client.SendTxn(txns[i], opts.Order, false)
		}
	} else {
		log.Infof("Start sending transactions")
		for i := 0; i < opts.Num && i < len(txns); i++ {
			client.SendTxn(txns[i], opts.Order, false)
		}
	}
	// send blocks (only for testing)
	if opts.SendBlock && opts.Order {
		time.Sleep(time.Duration(5)*time.Second)
		log.Infof("Start sending block")
		client.SendBlock(txns, opts.BlockSize)
	}
}

func RandomString(n int) string {
	var letter = []rune("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789")
	b := make([]rune, n)
	for i := range b {
		b[i] = letter[rand.Intn(len(letter))]
	}
	return string(b)
}

package infra

import (
	"bufio"
	"encoding/json"
	"fmt"
	"os"
	"sync/atomic"
	"time"

	"github.com/hyperledger/fabric-protos-go/common"
	log "github.com/sirupsen/logrus"
)

var endorsement_file = "ENDORSEMENT"

var (
	MAX_BUF           = 100010
	buffer_start      = make(chan string, MAX_BUF) // start: timestamp txid clientid connectionid
	buffer_proposal   = make(chan string, MAX_BUF) // proposal: timestamp txid
	buffer_sent       = make(chan string, MAX_BUF) // sent: timestamp txid
	buffer_end        = make(chan string, MAX_BUF) // end: timestamp txid [VALID/MVCC]
	buffer_tot        = make(chan string, MAX_BUF) // null
	g_num             int
	g_hot_rate        float64
	g_contetion_rate  float64
	g_txtype          string
	g_ndrate          float64
	g_num_of_conn     int
	g_client_per_conn int
	g_groups          int
	g_orderer_client  int
)

func print_benchmark() {
	// output log
	for len(buffer_start) > 0 {
		fmt.Println(<-buffer_start)
	}
	for len(buffer_proposal) > 0 {
		fmt.Println(<-buffer_proposal)
	}
	for len(buffer_sent) > 0 {
		fmt.Println(<-buffer_sent)
	}
	for len(buffer_end) > 0 {
		fmt.Println(<-buffer_end)
	}
	for len(buffer_tot) > 0 {
		fmt.Println(<-buffer_tot)
	}
}

func e2e(config Config, num int, burst int, rate float64, logger *log.Logger) error {
	g_num = num
	crypto, err := config.LoadCrypto()
	if err != nil {
		return err
	}
	raw := make(chan *Elements, burst)
	signed := make([]chan *Elements, len(config.Endorsers))
	processed := make(chan *Elements, burst)
	envs := make(chan *Elements, burst)
	done := make(chan struct{})
	finishCh := make(chan struct{})
	errorCh := make(chan error, burst)
	assember := &Assembler{Signer: crypto, EndorserGroups: g_groups, Conf: config}

	for i := 0; i < len(config.Endorsers); i++ {
		signed[i] = make(chan *Elements, burst)
	}

	for i := 0; i < config.Threads; i++ {
		go assember.StartIntegrator(processed, envs, errorCh, done)
	}

	proposor, err := CreateProposers(g_num_of_conn, g_client_per_conn, config.Endorsers, assember, logger)
	if err != nil {
		return err
	}
	proposor.Start(signed, envs, done, config)

	broadcaster, err := CreateBroadcasters(g_orderer_client, config.Orderer, logger)
	if err != nil {
		return err
	}
	broadcaster.Start(envs, errorCh, done)

	observer, err := CreateObserver(config.Channel, config.Committer, crypto, logger)
	if err != nil {
		return err
	}
	go StartCreateProposal(num, burst, rate, config, crypto, raw, errorCh, logger)

	time.Sleep(time.Duration(10) * time.Second)
	start := time.Now()
	go observer.Start(int32(num), errorCh, finishCh, start, &assember.Abort)
	for i := 0; i < config.Threads; i++ {
		go assember.StartSigner(raw, signed, errorCh, done)
	}

	for {
		select {
		case err = <-errorCh:
			return err
		case <-finishCh:
			duration := time.Since(start)
			close(done)
			// output log
			print_benchmark()

			logger.Infof("Completed processing transactions.")
			fmt.Printf("tx: %d, duration: %+v, tps: %f\n", num, duration, float64(num)/duration.Seconds())
			fmt.Printf("abort rate because of the different ledger height: %d %.2f%%\n", assember.Abort, float64(assember.Abort)/float64(num)*100)
			return nil
		}
	}
}

func breakdown_phase1(config Config, num int, burst int, rate float64, logger *log.Logger) error {
	crypto, err := config.LoadCrypto()
	if err != nil {
		return err
	}
	raw := make(chan *Elements, burst)
	signed := make([]chan *Elements, len(config.Endorsers))
	processed := make(chan *Elements, burst)
	done := make(chan struct{})
	errorCh := make(chan error, burst)
	assember := &Assembler{Signer: crypto, EndorserGroups: g_groups, Conf: config}

	for i := 0; i < len(config.Endorsers); i++ {
		signed[i] = make(chan *Elements, burst)
	}

	for i := 0; i < 5; i++ {
		go assember.StartSigner(raw, signed, errorCh, done)
	}

	proposor, err := CreateProposers(g_num_of_conn, g_client_per_conn, config.Endorsers, assember, logger)
	if err != nil {
		return err
	}
	proposor.Start(signed, processed, done, config)

	start := time.Now()
	go StartCreateProposal(num, burst, rate, config, crypto, raw, errorCh, logger)

	// phase1: send proposals to endorsers
	var cnt int32 = 0
	var buffer [][]byte
	for i := 0; i < num; i++ {
		select {
		case err = <-errorCh:
			return err
		case tx := <-processed:
			res, err := assember.Assemble(tx)
			if err != nil {
				atomic.AddInt32(&assember.Abort, 1)
				if cnt+assember.Abort >= int32(num) {
					break
				}
				continue
			}
			bytes, err := json.Marshal(res.Envelope)
			if err != nil {
				fmt.Println("error: marshal envelop")
				return err
			}
			cnt += 1
			buffer = append(buffer, bytes)
			if cnt+assember.Abort >= int32(num) {
				break
			}
		}
	}
	duration := time.Since(start)
	close(done)
	// output log
	print_benchmark()

	logger.Infof("Completed endorsing transactions.")
	fmt.Printf("tx: %d, duration: %+v, tps: %f\n", num, duration, float64(num)/duration.Seconds())
	fmt.Printf("abort rate because of the different ledger height: %d %.2f%%\n", assember.Abort, float64(assember.Abort)/float64(num)*100)
	// persistency
	mfile, _ := os.Create(endorsement_file)
	defer mfile.Close()
	mw := bufio.NewWriter(mfile)
	for i := range buffer {
		mw.Write(buffer[i])
		mw.WriteByte('\n')
	}
	mw.Flush()
	return nil
}
func breakdown_phase2(config Config, num int, burst int, rate float64, logger *log.Logger) error {
	crypto, err := config.LoadCrypto()
	if err != nil {
		return err
	}
	envs := make(chan *Elements, burst)
	done := make(chan struct{})
	finishCh := make(chan struct{})
	errorCh := make(chan error, burst)

	broadcaster, err := CreateBroadcasters(g_orderer_client, config.Orderer, logger)
	if err != nil {
		return err
	}
	broadcaster.Start(envs, errorCh, done)

	observer, err := CreateObserver(config.Channel, config.Committer, crypto, logger)
	if err != nil {
		return err
	}

	mfile, _ := os.Open(endorsement_file)
	defer mfile.Close()
	mscanner := bufio.NewScanner(mfile)
	TXs := make([]common.Envelope, num)
	i := 0
	for mscanner.Scan() {
		bytes := mscanner.Bytes()
		json.Unmarshal(bytes, &TXs[i])
		i++
	}
	start := time.Now()
	var temp0 int32 = 0
	go observer.Start(int32(num), errorCh, finishCh, start, &temp0)
	go func() {
		for i := 0; i < len(TXs); i++ {
			var item Elements
			item.Envelope = &TXs[i]
			envs <- &item
		}
	}()

	for {
		select {
		case err = <-errorCh:
			return err
		case <-finishCh:
			duration := time.Since(start)
			close(done)
			print_benchmark()

			logger.Infof("Completed processing transactions.")
			fmt.Printf("tx: %d, duration: %+v, tps: %f\n", num, duration, float64(num)/duration.Seconds())
			return nil
		}
	}

}

func Process(configPath string, num int, burst int, rate float64, e bool, hot_rate, contention_rate, nd_rate float64, txtype string, num_of_conn, client_per_conn int, groups int, orderer_client int, logger *log.Logger) error {
	g_hot_rate = hot_rate
	g_contetion_rate = contention_rate
	g_txtype = txtype
	g_ndrate = nd_rate
	g_num_of_conn = num_of_conn
	g_client_per_conn = client_per_conn
	g_groups = groups
	g_orderer_client = orderer_client
	config, err := LoadConfig(configPath)
	if err != nil {
		return err
	}
	config.End2end = e
	if config.End2end {
		fmt.Println("e2e")
		return e2e(config, num, burst, rate, logger)
	} else {
		if _, err := os.Stat(endorsement_file); err == nil {
			fmt.Println("phase2")
			// phase2: broadcast transactions to order
			return breakdown_phase2(config, num, burst, rate, logger)
		} else {
			fmt.Println("phase1")
			// phase1: send proposals to endorsers {
			return breakdown_phase1(config, num, burst, rate, logger)
		}

	}
}

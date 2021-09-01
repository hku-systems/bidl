package util

import (
	"time"

	log "github.com/sirupsen/logrus"
)

type TputMonitor struct {
	TputTxn chan int
	LatencyTxn chan int
	TputBlk chan int
}

var Monitor *TputMonitor

func NewTputMonitor(blkSize int) {
	Monitor = &TputMonitor{
		TputTxn: make(chan int),
		TputBlk: make(chan int),
	}
	go Monitor.TxnThroughput()
	go Monitor.BlkThroughput(blkSize)
}

func (monitor *TputMonitor) TxnThroughput() {
	log.Infof("Start monitor transaction receiving throughput")
	num := 0
	total := 0
	interval := 1000
	start := time.Now()
	for {
		select {
		case <-monitor.TputTxn:
			num++
			total++
			if num == interval {
				// duration := int(time.Since(start).Nanoseconds())
				duration := int(time.Since(start).Milliseconds())
				// log.Infof("Received %d transactions, duration: %dms, transaction tput: %d kTxns/s, total: %d", interval, duration, interval/duration, total)
				if duration == 0 {
					// fmt.Printf("BIDL transaction commit throughput: Inf kTxns/s\n")
					log.Infof("BIDL transaction commit throughput: Inf kTxns/s\n")
				} else {
					// fmt.Printf("BIDL transaction commit throughput: %d kTxns/s\n", interval/duration)
					log.Infof("BIDL transaction commit throughput: %d kTxns/s\n", interval/duration)
				}
				start = time.Now()
				num = 0
			}
		}
	}
}

func (monitor *TputMonitor) BlkThroughput(blkSize int) {
	log.Infof("Start monitor block committing throughput")
	num := 0
	interval := 2
	start := time.Now()
	for {
		select {
		case <-monitor.TputBlk:
			num++
			if num == interval {
				// duration := int(time.Since(start).Nanoseconds())
				duration := int(time.Since(start).Milliseconds())
				if duration == 0 {
					// fmt.Printf("BIDL block commit throughput: Inf kTxns/s\n")
					log.Infof("BIDL block commit throughput: Inf kTxns/s\n")
				} else {
					// fmt.Printf("BIDL block commit throughput: %d kTxns/s\n", interval*blkSize*1e6/duration)
					// fmt.Printf("BIDL block commit throughput: %d kTxns/s\n", interval*blkSize/duration)
					log.Infof("BIDL block commit throughput: %d kTxns/s\n", interval*blkSize/duration)
				}
				start = time.Now()
				num = 0
			}
		}
	}
}

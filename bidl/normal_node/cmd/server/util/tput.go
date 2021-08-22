package util

import (
	log "github.com/sirupsen/logrus"
	"time"
	"fmt"
)

type TputMonitor struct {
	TputTxn chan int
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
				duration := int(time.Since(start).Milliseconds())
				log.Infof("Received %d transactions, duration: %dms, transaction receiving tput: %d kTxns/s, total: %d", interval, duration, interval/duration, total)
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
				duration := int(time.Since(start).Nanoseconds())
				if duration == 0 {
					duration = 1
				}
				fmt.Printf("BIDL throughput: %d kTxns/s\n", interval*blkSize*1e6/duration)
				start = time.Now()
				num = 0
			}
		}
	}
}

package core

import (
	"strconv"

	"normal_node/cmd/server/util"

	log "github.com/sirupsen/logrus"
)

func (p *Processor) commitTxn(hashes [][32]byte) {
	log.Debugf("Commiting transaction state.")
	for i := 0; i < len(hashes); i++ {
		hash := hashes[i]
		if _, ok := p.Envelops[hash]; ok {
			env := p.Envelops[hash]
			wset := env.WSet
			if !env.ND {
				log.Debugf("The transaction is deterministic")
				for k, v := range wset {
					p.DB.Put([]byte(strconv.Itoa(k)), []byte(strconv.Itoa(v)), nil)
				}
				util.Monitor.TputTxn <- 1
			} else {
				log.Debugf("The transaction is non-deterministic")
			}
			delete(p.Envelops, hash)
		} else {
			log.Debugf("I don't hold the hash")
		}
	}
}

func (p *Processor) commitBlock(block []byte) {
	log.Debugf("Commiting block.")
	p.DB.Put([]byte("block"+strconv.Itoa(p.blkNum)), block, nil)
}

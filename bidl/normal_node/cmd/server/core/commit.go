package core

import (
	"strconv"

	log "github.com/sirupsen/logrus"
)

func (p *Processor) commitTxn(hashes [][32]byte) {
	log.Debugf("Commiting transaction state.")
	for i := 0; i < len(hashes); i++ {
		hash := hashes[i]
		if _, ok := p.Envelops[hash]; ok {
			env := p.Envelops[hash]
			wset := env.WSet
			// commit the write set to DB
			if !env.ND {
				for k, v := range wset {
					p.DB.Put([]byte(strconv.Itoa(k)), []byte(strconv.Itoa(v)), nil)
				}
			}
			// delete the writeset
			delete(p.Envelops, hash)
		}
	}
}

func (p *Processor) commitBlock(block []byte) {
	log.Debugf("Commiting block.")
	p.DB.Put([]byte("block"+strconv.Itoa(p.blkNum)), block, nil)
}

//// write state to levelDB, and delete the RWSets
//func (p *Processor) commitStateBySeq(txnNum uint64) {
//	p.mutex.Lock()
//	defer p.mutex.Unlock()
//	end := p.WaterMarkLow + txnNum
//	for i := p.WaterMarkLow; i < end; i++ {
//		env := p.Envelops[i]
//		wset := env.WSet
//		//wset := p.Envelops[i].WSet
//		for k, v := range wset {
//			p.DB.Put([]byte(strconv.Itoa(k)), []byte(strconv.Itoa(v)), nil)
//		}
//	}
//	p.WaterMarkLow = end
//}

package main

import (
	"math/rand"
	"normal_node/cmd/common"
	"normal_node/cmd/server/util"
	"os"
	"strconv"
	"time"

	log "github.com/sirupsen/logrus"
	"github.com/syndtr/goleveldb/leveldb"
)

// acc: account number
// org: organization number
// init: initial balance
// create acc number of accounts for each organization
func CreateAccounts(acc int, org int, bal int) {
	dbFile := "../state.db"
	err := os.RemoveAll(dbFile)
	if err != nil {
		log.Error("dbfile remove Error!")
	}
	start := time.Now()
	db, err := leveldb.OpenFile(dbFile, nil)
	defer db.Close()
	if err != nil {
		log.Fatal("Error open state.db!")
	}
	for i := 0; i < org; i++ {
		for j := 0; j < acc; j++ {
			// the string concatenation is the major performance bottleneck
			err = db.Put([]byte(strconv.Itoa(i)+":"+strconv.Itoa(j)), []byte(strconv.Itoa(bal)), nil)
		}
	}
	duration := int(time.Since(start).Milliseconds())
	log.Infof("Successfully generated %d accounts for %d organizations, duration %dms, tps: %d kTps", acc, org, duration, acc*org/duration)
}

// randomly select two organizations
// select one account from each organization
// the amount of money transfer is 1
// acc: number of accounts
// org: number of organizations
// num: number of transactions
// conflict: ratio of hot accounts
func GenerateTransferWorkload(acc int, org int, num int, conflict int) []*common.Transaction {
	conflictNum := 0
	if conflict > 0 {
		conflictNum = int(num / 100 * conflict)
		log.Infof("%d%% accounts are hot accounts", conflict)
	}

	txns := make([]*common.Transaction, num)
	dummy := make([]byte, 800)
	rand.Read(dummy)
	for i := 0; i < num; i++ {
		// orgID := rand.Intn(org)
		org1 := rand.Intn(org)
		org2 := (org1+1) % org
		acc1 := rand.Intn(acc)
		acc2 := rand.Intn(acc)
		// select accounts from the hot accounts
		if conflict > 0 {
			acc1 = rand.Intn(conflictNum)
			acc2 = rand.Intn(conflictNum)
		}
		payloadStr := strconv.Itoa(acc1) + ":" + strconv.Itoa(acc2) + "1"
		orgStr := strconv.Itoa(org1) + ":" + strconv.Itoa(org2) // org1:org2:org3
		// orgStr := strconv.Itoa(orgID) // org1:org2:org3
		sig := util.CreateMAC([]byte(payloadStr), []byte(common.SecretKey))
		txn := &common.Transaction {
			Type:      common.TxnMessage,
			Payload:   []byte(payloadStr),
			Org:       []byte(orgStr),
			Signature: sig,
			Dummy:     dummy,
		}
		txns[i] = txn
	}
	return txns
}

// GenerateCreateWorkload generate transactions for creating accounts
// acc: number of accounts for each organization
// org: number of organizations
// nd: non-determinism rate
func GenerateCreateWorkload(acc int, org int, nd int) []*common.Transaction {
	ndAcc := 0
	if nd > 0 {
		ndAcc = int(acc / 100 * nd)
		log.Infof("%d out of %d transactions (%d%%) are non-deterministic in each of the %d organizations", ndAcc, acc, nd, org)
	}
	txns := make([]*common.Transaction, acc*org)
	dummy := make([]byte, 800)
	rand.Read(dummy)
	for i := 0; i < org; i++ {
		ndNum := ndAcc
		for j := 0; j < acc; j++ {
			var payloadStr string
			if ndNum > 0 {
				payloadStr = strconv.Itoa(j) + ":" + strconv.Itoa(-1)
				ndNum--
			} else {
				payloadStr = strconv.Itoa(j) + ":" + strconv.Itoa(100000)
			}
			orgStr := strconv.Itoa(i) + ":" + strconv.Itoa((i+1) % org)
			// orgStr := strconv.Itoa(i)
			sig := util.CreateMAC([]byte(payloadStr), []byte(common.SecretKey))
			txn := &common.Transaction{
				Type:      common.TxnMessage,
				Payload:   []byte(payloadStr),
				Org:       []byte(orgStr),
				Signature: sig,
				Dummy:	   dummy,
			}
			txns[i*acc+j] = txn
		}
	}
	return txns
}


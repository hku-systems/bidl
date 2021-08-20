package main

import (
	log "github.com/sirupsen/logrus"
	"github.com/syndtr/goleveldb/leveldb"
	"math/rand"
	"normal_node/cmd/common"
	"normal_node/cmd/server/util"
	"os"
	"strconv"
	"time"
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
func GenerateTransferWorkload(acc int, org int, num int) []*common.Transaction {
	txns := make([]*common.Transaction, num)
	for i := 0; i < num; i++ {
		org1 := rand.Intn(org)
		acc1 := rand.Intn(acc)
		org2 := org1 + 1
		acc2 := rand.Intn(acc)
		payloadStr := strconv.Itoa(acc1) + ":" + strconv.Itoa(acc2) + "1"
		orgStr := strconv.Itoa(org1) + ":" + strconv.Itoa(org2)
		sig := util.CreateMAC([]byte(payloadStr), []byte(common.SecretKey))
		txn := &common.Transaction{
			Type:      common.TxnMessage,
			Payload:   []byte(payloadStr),
			Org:       []byte(orgStr),
			Signature: sig,
		}
		txns[i] = txn
	}
	return txns
}

// generate transactions for creating accounts
// acc: number of accounts for each organization
// org: number of organizations
// nd: non-determinism
func GenerateCreateWorkload(acc int, org int, nd bool) []*common.Transaction {
	txns := make([]*common.Transaction, acc*org)
	for i := 0; i < org; i++ {
		for j := 0; j < acc; j++ {
			var payloadStr string
			if nd {
				payloadStr = strconv.Itoa(j) + ":" + strconv.Itoa(-1)
			} else {
				payloadStr = strconv.Itoa(j) + ":" + strconv.Itoa(100000)
			}
			orgStr := strconv.Itoa(i) + ":" + strconv.Itoa((i+1)%org)
			sig := util.CreateMAC([]byte(payloadStr), []byte(common.SecretKey))
			txn := &common.Transaction{
				Type:      common.TxnMessage,
				Payload:   []byte(payloadStr),
				Org:       []byte(orgStr),
				Signature: sig,
			}
			txns[i*acc+j] = txn
		}
	}
	return txns
}

//func main() {
//	//CreateAccounts(10000, 4, 100000)
//	GenerateCreateWorkload(10000, 4)
//	GenerateTransferWorkload(10000, 4, 10000)
//}

package core

import (
	"bytes"
	"crypto/ecdsa"
	"crypto/elliptic"
	"crypto/rand"
	"crypto/sha256"
	"encoding/binary"
	"normal_node/cmd/common"
	"normal_node/cmd/server/config"
	"normal_node/cmd/server/network"
	"normal_node/cmd/server/util"
	"strconv"
	"strings"
	"sync"
	"time"
	"unsafe"

	log "github.com/sirupsen/logrus"
	"github.com/syndtr/goleveldb/leveldb"
	"github.com/vmihailenco/msgpack"
)

type Processor struct {
	Persists   map[[32]byte]int            // persist num
	TXPool     map[uint64]common.Envelop   // index transactions by sequence number
	Envelops   map[[32]byte]common.Envelop // index transactions by hashes
	DB         *leveldb.DB
	TempState  map[int]int // temp execution state
	txnNum     int         // total number of transactions received
	execNum    int         // total number of transactions executed
	blkNum     int         // total number of block received
	mutex      sync.Mutex
	BlkSize    int // default block size
	ID         int // node id
	Net        *network.Network
	privateKey *ecdsa.PrivateKey
}

var startExecBlk time.Time
var elapsedBlk time.Duration

func NewProcessor(blkSize int, id int, net *network.Network) *Processor {
	log.Debugf("Transaction processor initialized")
	dbFile := "state.db"
	database, err := leveldb.OpenFile(dbFile, nil)
	if err != nil {
		log.Fatal("Error open state.db!")
	}

	pubkeyCurve := elliptic.P256()
	privatekey := new(ecdsa.PrivateKey)
	privatekey, err = ecdsa.GenerateKey(pubkeyCurve, rand.Reader) // this generates a public & private key pair
	// privateKey, err := ecdsa.GenerateKey(rand.Reader, 2048)
	if err != nil {
		panic(err)
	}

	return &Processor{
		Persists:   make(map[[32]byte]int),
		TXPool:     make(map[uint64]common.Envelop),
		Envelops:   make(map[[32]byte]common.Envelop),
		DB:         database,
		TempState:  make(map[int]int),
		txnNum:     0,
		execNum:    0,
		blkNum:     0,
		BlkSize:    blkSize,
		ID:         id,
		Net:        net,
		privateKey: privatekey,
	}
}

func (p *Processor) Related(orgs []byte) bool {
	if p.ID == -1 {
		return true
	}
	orgStr := string(orgs)
	orgsRelate := strings.Split(orgStr, ":")
	related := false
	for _, org := range orgsRelate {
		orgInt, _ := strconv.Atoi(org)
		if orgInt == p.ID {
			related = true
			break
		}
	}
	return related
}

// execute transaction
func (p *Processor) ProcessTxn(txn *common.SequencedTransaction) {
	p.mutex.Lock()
	defer p.mutex.Unlock()

	if p.txnNum == 0 {
		// startExecBlk = time.Now()
		elapsedBlk = 0
	}
	startExecTxn := time.Now()
	p.txnNum++
	if p.Related(txn.Transaction.Org) {
		p.execNum++
	}

	// verify MAC
	sig := txn.Transaction.Signature[:32]
	valid := util.ValidMAC(txn.Transaction.Payload, sig, []byte(common.SecretKey))
	if !valid {
		log.Infof("MAC for transaction %d is invalid, discard the transaction", txn.Seq)
		return
	}
	// execute transaction
	p.ExecuteTxn(txn)
	elapsedTxn := time.Since(startExecTxn) / time.Microsecond // duration in us
	elapsedBlk += elapsedTxn
	if p.txnNum%p.BlkSize == 0 {
		log.Infof("Execution latency: %d ms, for executing %d transactions.\n", elapsedBlk/1e3, p.execNum)
		elapsedBlk = 0
		p.execNum = 0
	}
}

func (p *Processor) ExecuteTxn(txn *common.SequencedTransaction) {
	related := p.Related(txn.Transaction.Org)
	payload := strings.Split(string(txn.Transaction.Payload), ":")
	var env common.Envelop
	nd := false
	if related {
		if len(payload) == 2 { // account creation transaction
			acc, _ := strconv.Atoi(payload[0])
			balance, _ := strconv.Atoi(payload[1])
			env = p.ExecCreateTxn(acc, balance, txn)
		} else if len(payload) == 3 { // balance transfer transaction
			from, _ := strconv.Atoi(payload[0])
			to, _ := strconv.Atoi(payload[1])
			balance, _ := strconv.Atoi(payload[2])
			env = p.ExecTransferTxn(from, to, balance, txn)
		} else {
			log.Errorf("Error format of transactions")
		}

		buf, _ := msgpack.Marshal(env)
		p.DB.Put([]byte("tx:"+strconv.Itoa(p.txnNum)), buf, nil)
		msgHash := sha256.New()
		_, err := msgHash.Write(buf)
		if err != nil {
			panic(err)
		}

		msgHashSum := msgHash.Sum(nil)
		r, s, err := ecdsa.Sign(rand.Reader, p.privateKey, msgHashSum)
		if err != nil {
			panic(err)
		}
		signature := r.Bytes()
		signature = append(signature, s.Bytes()...)
		env.Signature = signature
		p.PersistExecResult(&env)
	} else {
		balance := 0
		if len(payload) == 2 {
			balance, _ = strconv.Atoi(payload[1])
		} else if len(payload) == 3 {
			balance, _ = strconv.Atoi(payload[2])
		} else {
			log.Errorf("Error format transaction")
			return
		}

		if balance == -1 {
			nd = true
		}
		env = common.Envelop{
			SeqTransaction: txn,
			WSet:           make(map[int]int),
			ND:             nd,
		}
	}

	p.Envelops[txn.Hash] = env
	p.TXPool[txn.Seq] = env
}

func (p *Processor) ExecCreateTxn(account int, balance int, txn *common.SequencedTransaction) common.Envelop {
	log.Debugf("Executing create transaction, account: %d, balance: %d", account, balance)
	if _, ok := p.TempState[account]; ok {
		log.Debugf("Account already exist")
	} else {
		p.TempState[account] = balance
	}
	nd := false
	if balance == -1 {
		nd = true
	}
	// generate WSet
	wset := map[int]int{
		account: p.TempState[account],
	}
	buf, _ := msgpack.Marshal(wset)
	execHash := sha256.Sum256(buf)
	env := common.Envelop{
		SeqTransaction: txn,
		WSet:           wset,
		ExecHash:       execHash,
		ND:             nd,
	}
	return env
}

func (p *Processor) ExecTransferTxn(from int, to int, balance int, txn *common.SequencedTransaction) common.Envelop {
	log.Debugf("Executing transfer transaction, from: %d, to: %d, balance: %d", from, to, balance)
	// update state
	if balanceState, ok := p.TempState[from]; ok {
		p.TempState[from] = balanceState - balance
	} else {
		balanceDBStr, err := p.DB.Get([]byte(strconv.Itoa(from)), nil)
		util.ErrorCheck(err, "ExecuteTxn", true)
		balanceDB, err := strconv.Atoi(string(balanceDBStr))
		util.ErrorCheck(err, "ExecuteTxn", true)
		p.TempState[from] = balanceDB - balance
	}
	if balanceState, ok := p.TempState[to]; ok {
		p.TempState[to] = balanceState + balance
	} else {
		balanceDBStr, err := p.DB.Get([]byte(strconv.Itoa(to)), nil)
		util.ErrorCheck(err, "ExecuteTxn", true)
		balanceDB, err := strconv.Atoi(string(balanceDBStr))
		util.ErrorCheck(err, "ExecuteTxn", true)
		p.TempState[to] = balanceDB + balance
	}
	// generate envelop
	wset := map[int]int{
		from: p.TempState[from],
		to:   p.TempState[to],
	}
	buf, _ := msgpack.Marshal(wset)
	execHash := sha256.Sum256(buf)
	env := common.Envelop{
		SeqTransaction: txn,
		WSet:           wset,
		ExecHash:       execHash,
		ND:             false,
	}
	return env
}

// retrieve transaction hashes from the block bytes, and compare hashes
func (p *Processor) ProcessBlock(block []byte) {
	p.mutex.Lock()
	defer p.mutex.Unlock()
	// the first four bytes is an int indicating the block length
	// the last eight bytes is two ints indicating the maximum sequence number and the signature length (0)
	payload := block[4 : len(block)-8]
	hashNum := len(payload) / 36
	if len(payload)%36 != 0 {
		log.Errorf("The block is badly formed, block length: %d.", len(payload))
		return
	}
	log.Debugf("New block received, block number: %d", p.blkNum)
	startBlkCommit := time.Now()
	// obtain transaction [seq, hash] from block
	hashesBlk := make([][]byte, 0)
	hashes := make([][32]byte, 0)
	var seqHash []byte
	for i := 0; i < hashNum; i++ {
		// seqHash = byte32(payload[i*36 : i*36+36])
		seqHash = payload[i*36 : i*36+36]
		hashesBlk = append(hashesBlk, seqHash)
		hashes = append(hashes, *byte32(payload[i*36+4 : i*36+36]))
	}
	// check transaction hashes
	reExec := false
	for i := 0; i < hashNum; i++ {
		seq := binary.LittleEndian.Uint32(hashesBlk[i][:4])
		hashBlk := hashesBlk[i][4:]
		if _, ok := p.TXPool[uint64(seq)]; ok {
			hashLocal := p.TXPool[uint64(seq)].SeqTransaction.Hash
			//log.Debugf("sequence number is %l, hashnum: %d, blkNum %d, i: %d", seq, hashNum, p.blkNum, i)
			//log.Debugf("hashBlk", hashBlk, "hashLocal", hashLocal)
			if !bytes.Equal(hashBlk[:], hashLocal[:]) {
				reExec = true
				break
			}
		}
	}
	// mismatch hash! re-execute transactions in the block
	if reExec {
		log.Infof("re-execute transactions.")
		for i := 0; i < hashNum; i++ {
			hashBlk := *byte32(hashesBlk[i][4:])
			if _, ok := p.Envelops[hashBlk]; ok {
				p.ExecuteTxn(p.Envelops[hashBlk].SeqTransaction)
			}
		}
	}
	p.blkNum++
	p.commitTxn(hashes)
	p.commitBlock(block)
	elapsed := time.Since(startBlkCommit) / time.Millisecond // duration in ms
	// fmt.Printf("Commit latency: %d ms for block %d.\n", elapsed, p.blkNum)
	log.Infof("Commit latency: %d ms for block %d.\n", elapsed, p.blkNum)
}

func byte32(s []byte) (a *[32]byte) {
	if len(a) <= len(s) {
		a = (*[len(a)]byte)(unsafe.Pointer(&s[0]))
	}
	return a
}

// send the execution result of a transaction to the coordinator
func (p *Processor) ExchangeExecResult(env *common.Envelop) {
	// get the coordinator of a transaction
	orgsStr := string(env.SeqTransaction.Transaction.Org)
	orgs := strings.Split(orgsStr, ":")
	coord, _ := strconv.Atoi(orgs[0])
	if coord == p.ID {
		return
	}

	// prepare result msg
	buf, _ := msgpack.Marshal(env.WSet)
	execHash := sha256.Sum256(buf)
	result := &common.Result{
		Org:      env.SeqTransaction.Transaction.Org,
		Seq:      env.SeqTransaction.Seq,
		ExecHash: execHash,
		TxnHash:  env.ExecHash,
	}
	buf, _ = msgpack.Marshal(result)
	buf = append(common.MagicNumExecExchange, buf...)

	// send to the coordinator
	p.Net.Send(buf, config.Hosts[coord])
}

// broadcast execution results
func (p *Processor) PersistExecResult(env *common.Envelop) {
	result := &common.Result{
		Org:      env.SeqTransaction.Transaction.Org,
		Seq:      env.SeqTransaction.Seq,
		ExecHash: env.ExecHash,
		TxnHash:  env.SeqTransaction.Hash,
	}
	buf, _ := msgpack.Marshal(result)
	buf = append(common.MagicNumExecResult, buf...)
	log.Debugf("Persisting execution result for transaction %d", env.SeqTransaction.Seq)
	p.Net.Multicast(buf)
}

func (p *Processor) ProcessPersist(data []byte) {
	p.mutex.Lock()
	defer p.mutex.Unlock()
	var result common.Result
	msgpack.Unmarshal(data, &result)
	log.Debugf("New persist message received, seq: %d", result.Seq)
	var hashes [1][32]byte
	hashes[0] = result.TxnHash
	if _, ok := p.Persists[result.TxnHash]; ok {
		p.Persists[result.TxnHash] = p.Persists[result.TxnHash] + 1
	} else {
		p.Persists[result.TxnHash] = 1
	}
	if p.Persists[result.TxnHash] == 3 { // 3f+1 = 4
		log.Debugf("Transaction execution result persisted, seq: %d", result.Seq)
	}
}

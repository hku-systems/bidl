package common

type MessageType int

const (
	TxnMessage MessageType = iota
	BlkMessage
)

//some bytes associated with an address
type Packet struct {
	Bytes         []byte
}

type Message struct {
	Type    MessageType
	Message []byte
}

type Transaction struct {
	Type      MessageType
	Payload   []byte
	Org       []byte
	Signature []byte
	Dummy	  []byte
}

// transaction with sequence number
type SequencedTransaction struct {
	Transaction *Transaction
	Seq         uint64
	Hash        [32]byte
}

// execution results
type Result struct {
	Org  		[]byte
	Seq  		uint64
	ExecHash 	[32]byte
	TxnHash 	[32]byte
}

// sequenced transaction with execution results
type Envelop struct {
	SeqTransaction *SequencedTransaction
	WSet           map[int]int // execution result
	ExecHash       [32]byte    // hash of the WSet
	Signature      []byte 
	ND             bool
}

var SecretKey = "4234kxzjcjj3nxnxbcvsjfj"

var MagicNumExecExchange = []byte{3, 3, 3, 3}
var MagicNumPersist = []byte{4, 4, 4, 4}
var MagicNumExecResult = []byte{5, 5, 5, 5}
var MagicNumBlock = []byte{6, 6, 6, 6}
var MagicNumTxn = []byte{7, 7, 7, 7}
var MagicNumTxnMalicious = []byte{8, 8, 8, 8}

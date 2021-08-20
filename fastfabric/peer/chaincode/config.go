package chaincode

import (
	"context"
	"io/ioutil"
	"time"

	"gopkg.in/yaml.v2"
	"github.com/gogo/protobuf/proto"
	"github.com/hyperledger/fabric/protos/msp"
	"github.com/hyperledger/fabric/protos/orderer"
	"github.com/hyperledger/fabric/protos/peer"
	"github.com/hyperledger/fabric/core/comm"


)


type Config struct {
	Endorsers     []Node   `yaml:"endorsers"`
	Committer     Node     `yaml:"committer"`
	Orderer       Node     `yaml:"orderer"`
	Channel       string   `yaml:"channel"`
	Chaincode     string   `yaml:"chaincode"`
	Version       string   `yaml:"version"`
	Args          []string `yaml:"args"`
	MSPID         string   `yaml:"mspid"`
	PrivateKey    string   `yaml:"private_key"`
	SignCert      string   `yaml:"sign_cert"`
	NumOfConn     int      `yaml:"num_of_conn"`
	ClientPerConn int      `yaml:"client_per_conn"`
	TXs 		  int      `yaml:"TXs"`
	EndorsementOnly	bool 	`yaml:"endorsementOnly"`
	NumOfConnForOrderer     int      `yaml:"num_of_conn_for_orderer"`
	NumOfAssembler int      `yaml:"num_of_assembler"`
	RawRoutine int      `yaml:"raw_routine"`
	Buffer int 		`yaml:"buffer"`

}

type Node struct {
	Addr      string `yaml:"addr"`
	// TLSCACert string `yaml:"tls_ca_cert"`
}

func LoadConfig(f string) Config {
	raw, err := ioutil.ReadFile(f)
	if err != nil {
		panic(err)
	}

	config := Config{}
	if err = yaml.Unmarshal(raw, &config); err != nil {
		panic(err)
	}

	return config
}

func (c Config) LoadCrypto() *Crypto {

	conf := CryptoConfig{
		MSPID:    c.MSPID,
		PrivKey:  c.PrivateKey,
		SignCert: c.SignCert,
	}

	priv, err := GetPrivateKey(conf.PrivKey)
	if err != nil {
		panic(err)
	}

	cert, certBytes, err := GetCertificate(conf.SignCert)
	if err != nil {
		panic(err)
	}

	id := &msp.SerializedIdentity{
		Mspid:   conf.MSPID,
		IdBytes: certBytes,
	}

	name, err := proto.Marshal(id)
	if err != nil {
		panic(err)
	}

	return &Crypto{
		Creator:  name,
		PrivKey:  priv,
		SignCert: cert,
	}
}



func CreateGRPCClient() (*comm.GRPCClient, error) {
	config := comm.ClientConfig{}
	config.Timeout = 15 * time.Second


	grpcClient, err := comm.NewGRPCClient(config)
	if err != nil {
		return nil, err
	}

	return grpcClient, nil
}

func CreateEndorserClient(addr string) (peer.EndorserClient, error) {
	gRPCClient, err := CreateGRPCClient()
	if err != nil {
		return nil, err
	}

	conn, err := gRPCClient.NewConnection(addr, "endorserClient")
	if err != nil {
		return nil, err
	}

	return peer.NewEndorserClient(conn), nil
}

func CreateBroadcastClient(addr string) (orderer.AtomicBroadcast_BroadcastClient, error) {
	gRPCClient, err := CreateGRPCClient()
	if err != nil {
		return nil, err
	}

	conn, err := gRPCClient.NewConnection(addr, "broadcasrClient")
	if err != nil {
		return nil, err
	}

	return orderer.NewAtomicBroadcastClient(conn).Broadcast(context.Background())
}

func CreateDeliverFilteredClient(addr string) (peer.Deliver_DeliverFilteredClient, error) {
	gRPCClient, err := CreateGRPCClient()
	if err != nil {
		return nil, err
	}

	conn, err := gRPCClient.NewConnection(addr, "DeliverFilteredClient")
	if err != nil {
		return nil, err
	}

	return peer.NewDeliverClient(conn).DeliverFiltered(context.Background())
}


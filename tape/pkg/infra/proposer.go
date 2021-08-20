package infra

import (
	"context"
	"fmt"
	"io"
	"sync/atomic"
	"time"

	"github.com/hyperledger/fabric-protos-go/common"
	"github.com/hyperledger/fabric-protos-go/orderer"
	"github.com/hyperledger/fabric-protos-go/peer"
	"github.com/pkg/errors"
	log "github.com/sirupsen/logrus"
)

type Proposers struct {
	workers [][]*Proposer
	//one proposer per connection per peer
	client int
	logger *log.Logger
}

func CreateProposers(conn, client int, nodes []Node, assember *Assembler, logger *log.Logger) (*Proposers, error) {
	var ps [][]*Proposer
	var err error
	//one proposer per connection per peer
	for _, node := range nodes {
		row := make([]*Proposer, conn)
		for j := 0; j < conn; j++ {
			row[j], err = CreateProposer(node, assember, logger)
			if err != nil {
				return nil, err
			}
		}
		ps = append(ps, row)
	}

	return &Proposers{workers: ps, client: client, logger: logger}, nil
}

func (ps *Proposers) Start(signed []chan *Elements, processed chan *Elements, done <-chan struct{}, config Config) {
	ps.logger.Infof("Start sending transactions.")
	for i := 0; i < len(config.Endorsers); i++ {
		// peer connection should be config.ClientPerConn * config.NumOfConn
		for k := 0; k < g_client_per_conn; k++ {
			for j := 0; j < g_num_of_conn; j++ {
				tempk := k
				tempj := j
				go ps.workers[i][j].Start(signed[i], processed, done, int(len(config.Endorsers)/g_groups), tempk, tempj)
			}
		}
	}
}

type Proposer struct {
	e      peer.EndorserClient
	Addr   string
	assm   *Assembler
	logger *log.Logger
}

func CreateProposer(node Node, assember *Assembler, logger *log.Logger) (*Proposer, error) {
	endorser, err := CreateEndorserClient(node, logger)
	if err != nil {
		return nil, err
	}

	return &Proposer{e: endorser, Addr: node.Addr, assm: assember, logger: logger}, nil
}

func (p *Proposer) Start(signed, processed chan *Elements, done <-chan struct{}, threshold int, ii int, jj int) {
	for {
		select {
		case s := <-signed:
			//send sign proposal to peer for endorsement
			// todo
			st := time.Now().UnixNano()
			buffer_start <- fmt.Sprintf("start: %d %s %d %d", st, s.Txid, ii, jj)
			r, err := p.e.ProcessProposal(context.Background(), s.SignedProp)
			if err != nil || r.Response.Status < 200 || r.Response.Status >= 400 {
				if r == nil {
					p.logger.Errorf("Err processing proposal: %s, status: unknown, addr: %s \n", err, p.Addr)
				} else {
					p.logger.Errorf("Err processing proposal: %s, status: %d, message: %s, addr: %s \n", err, r.Response.Status, r.Response.Message, p.Addr)
				}
				continue
			}
			s.lock.Lock()
			//collect for endorsement
			s.Responses = append(s.Responses, r)
			if len(s.Responses) >= threshold {
				// processed <- s
				// todo
				st := time.Now().UnixNano()
				buffer_proposal <- fmt.Sprintf("proposal: %d %s", st, s.Txid)
				env, err := CreateSignedTx(s.Proposal, p.assm.Signer, s.Responses, p.assm.Conf.Check_rwset)
				if err != nil {
					atomic.AddInt32(&p.assm.Abort, 1)
					continue
				}
				s.Envelope = env
				processed <- s
				// st = time.Now().UnixNano()
				// buffer_sent <- fmt.Sprintf("sent: %d %s", st, s.Txid)
			}
			s.lock.Unlock()
		case <-done:
			return
		}
	}
}

type Broadcasters []*Broadcaster

func CreateBroadcasters(conn int, orderer Node, logger *log.Logger) (Broadcasters, error) {
	bs := make(Broadcasters, conn)
	for i := 0; i < conn; i++ {
		broadcaster, err := CreateBroadcaster(orderer, logger)
		if err != nil {
			return nil, err
		}
		bs[i] = broadcaster
	}

	return bs, nil
}

func (bs Broadcasters) Start(envs <-chan *Elements, errorCh chan error, done <-chan struct{}) {
	for _, b := range bs {
		go b.StartDraining(errorCh)
		go b.Start(envs, errorCh, done)
	}
}

type Broadcaster struct {
	c      orderer.AtomicBroadcast_BroadcastClient
	logger *log.Logger
}

func CreateBroadcaster(node Node, logger *log.Logger) (*Broadcaster, error) {
	client, err := CreateBroadcastClient(node, logger)
	if err != nil {
		return nil, err
	}

	return &Broadcaster{c: client, logger: logger}, nil
}

func (b *Broadcaster) Start(envs <-chan *Elements, errorCh chan error, done <-chan struct{}) {
	b.logger.Debugf("Start sending broadcast")
	for {
		select {
		case e := <-envs:
			// todo
			st := time.Now().UnixNano()
			buffer_sent <- fmt.Sprintf("sent: %d %s", st, e.Txid)
			err := b.c.Send(e.Envelope)
			if err != nil {
				errorCh <- err
			}

		case <-done:
			return
		}
	}
}

func (b *Broadcaster) StartDraining(errorCh chan error) {
	for {
		res, err := b.c.Recv()
		if err != nil {
			if err == io.EOF {
				return
			}
			b.logger.Errorf("recv broadcast err: %+v, status: %+v\n", err, res)
			return
		}

		if res.Status != common.Status_SUCCESS {
			errorCh <- errors.Errorf("recv errouneous status %s", res.Status)
			return
		}
	}
}

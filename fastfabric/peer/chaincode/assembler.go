package chaincode

import (
	"fmt"
	"sync"
	"time"

	"github.com/hyperledger/fabric/protos/common"
	"github.com/hyperledger/fabric/protos/peer"
)

type Elements struct {
	Proposal   *peer.Proposal
	SignedProp *peer.SignedProposal
	Responses  []*peer.ProposalResponse
	lock       sync.Mutex
	Envelope   *common.Envelope
}

type Assembler struct {
	Signer *Crypto
	idx int
}

func (a *Assembler) assemble(e *Elements) *Elements {
	env, err := CreateSignedTx(e.Proposal, a.Signer, e.Responses)
	if err != nil {
		panic(err)
	}
	e.Envelope = env
	return e
}

func (a *Assembler) sign(e *Elements) *Elements {
	sprop, err := SignProposal(e.Proposal, a.Signer)
	if err != nil {
		panic(err)
	}
	e.SignedProp = sprop

	return e
}

func (a *Assembler) StartDropping(raw chan *Elements, done <-chan struct{}) {
	cnt := 0
	for {
		select {
		case <-raw:
			cnt += 1
			if cnt % 1000 == 0 {
				fmt.Println("drop 1000 TXs")
			}

		case <-done:
			return
		}
	}

}

func (a *Assembler) StartSigner(raw chan *Elements, signed []chan *Elements, done <-chan struct{}, idx int, num int) {
	st := time.Now()
	cnt := 0
	cur := 0
	for {
		select {
		case r := <-raw:
			t := a.sign(r)
			cnt += 1
			ed := time.Now()
			interval := ed.Sub(st).Seconds()
			if interval > 2 {
				fmt.Printf("jyp: signed %.3f tps %d %f\n", float64(cnt)/interval, cnt, interval)
				st = ed
				cnt = 0
			}
			flag := false
			for{
				// v := signed[rand.Intn(len(signed))]  // random select
				v := signed[cur]
				cur += 1
				if cur >= len(signed) {
					cur = 0
				}
				select {
				case v <- t:
					flag = true
				default:
					continue
				}
				if flag {
					break
				}
			}
		case <-done:
			return
		}
	}
}

func (a *Assembler) StartIntegrator(processed, envs chan *Elements, done <-chan struct{}) {
	cnt := 0
	st := time.Now()
	for {
		select {
		case p := <-processed:
			envs <- a.assemble(p)
			cnt += 1
			ed := time.Now()
			interval := ed.Sub(st).Seconds()
			if interval > 2 {
				fmt.Printf("jyp: assembled %.3f tps %d %f\n", float64(cnt)/interval, cnt, interval)
				st = ed
				cnt = 0
			}
		case <-done:
			return
		}
	}
}

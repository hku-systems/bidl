package infra

import (
	"math/rand"
	"sync"
	"sync/atomic"

	"github.com/hyperledger/fabric-protos-go/common"
	"github.com/hyperledger/fabric-protos-go/peer"
)

type Elements struct {
	Proposal   *peer.Proposal
	SignedProp *peer.SignedProposal
	Responses  []*peer.ProposalResponse
	lock       sync.Mutex
	Envelope   *common.Envelope
	Txid       string
}

type Assembler struct {
	Signer         *Crypto
	EndorserGroups int
	Abort          int32
	Conf           Config
}

func (a *Assembler) Assemble(e *Elements) (*Elements, error) {
	e, err := a.assemble(e)
	return e, err
}
func (a *Assembler) assemble(e *Elements) (*Elements, error) {
	env, err := CreateSignedTx(e.Proposal, a.Signer, e.Responses, a.Conf.Check_rwset)
	if err != nil {
		return nil, err
	}
	e.Envelope = env
	return e, nil
}

func (a *Assembler) sign(e *Elements) (*Elements, error) {
	sprop, err := SignProposal(e.Proposal, a.Signer)
	if err != nil {
		return nil, err
	}
	e.SignedProp = sprop

	return e, nil
}

func (a *Assembler) StartSigner(raw chan *Elements, signed []chan *Elements, errorCh chan error, done <-chan struct{}) {
	endorsers_per_group := int(len(signed) / a.EndorserGroups)
	rand.Seed(666)
	for {
		select {
		case r := <-raw:
			t, err := a.sign(r)
			if err != nil {
				errorCh <- err
				return
			}
			// TODO: select endorser group based on transactions
			group := rand.Intn(a.EndorserGroups)
			st := int(endorsers_per_group * group)
			ed := st + endorsers_per_group
			for i := st; i < ed; i++ {
				signed[i] <- t
			}

		case <-done:
			return
		}
	}
}

func (a *Assembler) StartIntegrator(processed, envs chan *Elements, errorCh chan error, done <-chan struct{}) {
	for {
		select {
		case p := <-processed:
			e, err := a.assemble(p)
			if err != nil {
				// abort directly because of the different endorsement
				atomic.AddInt32(&a.Abort, 1)
				continue
				// errorCh <- err
				// return
			}
			envs <- e
		case <-done:
			return
		}
	}
}

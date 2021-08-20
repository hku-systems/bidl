/*
Copyright IBM Corp. 2016 All Rights Reserved.

SPDX-License-Identifier: Apache-2.0
*/

package txvalidator

import (
	"context"
	"fmt"
	"time"

	"github.com/golang/protobuf/proto"
	"github.com/hyperledger/fabric/common/channelconfig"
	"github.com/hyperledger/fabric/common/configtx"
	"github.com/hyperledger/fabric/common/flogging"
	"github.com/hyperledger/fabric/core/chaincode/platforms"
	"github.com/hyperledger/fabric/core/chaincode/platforms/golang"
	"github.com/hyperledger/fabric/core/common/sysccprovider"
	"github.com/hyperledger/fabric/core/ledger"
	ledgerUtil "github.com/hyperledger/fabric/core/ledger/util"
	"github.com/hyperledger/fabric/msp"
	"github.com/hyperledger/fabric/protos/common"
	mspprotos "github.com/hyperledger/fabric/protos/msp"
	"github.com/hyperledger/fabric/protos/peer"
	"github.com/hyperledger/fabric/protos/utils"
	"github.com/pkg/errors"
)

// Support provides all of the needed to evaluate the VSCC
type Support interface {
	// Acquire implements semaphore-like acquire semantics
	Acquire(ctx context.Context, n int64) error

	// Release implements semaphore-like release semantics
	Release(n int64)

	// Ledger returns the ledger associated with this validator
	Ledger() ledger.PeerLedger

	// MSPManager returns the MSP manager for this channel
	MSPManager() msp.MSPManager

	// Apply attempts to apply a configtx to become the new config
	Apply(configtx *common.ConfigEnvelope) error

	// GetMSPIDs returns the IDs for the application MSPs
	// that have been defined in the channel
	GetMSPIDs(cid string) []string

	// Capabilities defines the capabilities for the application portion of this channel
	Capabilities() channelconfig.ApplicationCapabilities
}

//Validator interface which defines API to validate block transactions
// and return the bit array mask indicating invalid transactions which
// didn't pass validation.
type Validator interface {
	Validate(block *common.Block) error
}

// private interface to decouple tx validator
// and vscc execution, in order to increase
// testability of TxValidator
type vsccValidator interface {
	VSCCValidateTx(seq int, payload *common.Payload, envBytes []byte, block *common.Block) (error, peer.TxValidationCode)
}

// implementation of Validator interface, keeps
// reference to the ledger to enable tx simulation
// and execution of vscc
type TxValidator struct {
	ChainID string
	Support Support
	Vscc    vsccValidator
}

var logger = flogging.MustGetLogger("committer/txvalidator")

type blockValidationRequest struct {
	block *common.Block
	d     []byte
	tIdx  int
}

type blockValidationResult struct {
	tIdx                 int
	validationCode       peer.TxValidationCode
	txsChaincodeName     *sysccprovider.ChaincodeInstance
	txsUpgradedChaincode *sysccprovider.ChaincodeInstance
	err                  error
	txid                 string
}

// NewTxValidator creates new transactions validator
func NewTxValidator(chainID string, support Support, sccp sysccprovider.SystemChaincodeProvider, pm PluginMapper) *TxValidator {
	// Encapsulates interface implementation
	pluginValidator := NewPluginValidator(pm, support.Ledger(), &dynamicDeserializer{support: support}, &dynamicCapabilities{support: support})
	return &TxValidator{
		ChainID: chainID,
		Support: support,
		Vscc:    newVSCCValidator(chainID, support, sccp, pluginValidator)}
}

func (v *TxValidator) chainExists(chain string) bool {
	// TODO: implement this function!
	return true
}

// Validate performs the validation of a block. The validation
// of each transaction in the block is performed in parallel.
// The approach is as follows: the committer thread starts the
// tx validation function in a goroutine (using a semaphore to cap
// the number of concurrent validating goroutines). The committer
// thread then reads results of validation (in orderer of completion
// of the goroutines) from the results channel. The goroutines
// perform the validation of the txs in the block and enqueue the
// validation result in the results channel. A few note-worthy facts:
// 1) to keep the approach simple, the committer thread enqueues
//    all transactions in the block and then moves on to reading the
//    results.
// 2) for parallel validation to work, it is important that the
//    validation function does not change the state of the system.
//    Otherwise the order in which validation is perform matters
//    and we have to resort to sequential validation (or some locking).
//    This is currently true, because the only function that affects
//    state is when a config transaction is received, but they are
//    guaranteed to be alone in the block. If/when this assumption
//    is violated, this code must be changed.
func (v *TxValidator) Validate(block *common.Block) error {
	// fmt.Println("jyp: validate tx (trace -1)")

	startValidation := time.Now() // timer to log Validate block duration
	logger.Debugf("[%s] START Block Validation for block [%d]", v.ChainID, block.Header.Number)

	// Initialize trans as valid here, then set invalidation reason code upon invalidation below
	txsfltr := ledgerUtil.NewTxValidationFlags(len(block.Data.Data))
	// txsChaincodeNames records all the invoked chaincodes by tx in a block
	// qiji: remove test
	/*txsChaincodeNames := make(map[int]*sysccprovider.ChaincodeInstance)*/
	// upgradedChaincodes records all the chaincodes that are upgraded in a block
	// qiji: remove test
	/*txsUpgradedChaincodes := make(map[int]*sysccprovider.ChaincodeInstance)*/
	// array of txids
	// qiji: remove test
	/*txidArray := make([]string, len(block.Data.Data))*/

	logger.Debugf("expecting %d block validation responses", len(block.Data.Data))

	for i := 0; i < len(block.Data.Data); i++ {
		tIdx := i
		txsfltr.SetFlag(tIdx, peer.TxValidationCode_VALID)
	}
	// now we read responses in the order in which they come back
	// qiji: remove test
	/*for i := 0; i < len(block.Data.Data); i++ {
		d := block.Data.Data[i]
		tIdx := i
		var txID string
		if env, err := utils.GetEnvelopeFromBlock(d); err != nil {
			txsfltr.SetFlag(tIdx, peer.TxValidationCode_INVALID_OTHER_REASON)
			continue
		} else if env != nil {
			var payload *common.Payload
			var err error
			var txsChaincodeName *sysccprovider.ChaincodeInstance
			var txsUpgradedChaincode *sysccprovider.ChaincodeInstance
			payload, err = utils.GetPayload(env)
			chdr, err := utils.UnmarshalChannelHeader(payload.Header.ChannelHeader)
			if err != nil {
				txsfltr.SetFlag(tIdx, peer.TxValidationCode_INVALID_OTHER_REASON)
				continue
			}

			if common.HeaderType(chdr.Type) == common.HeaderType_ENDORSER_TRANSACTION {
				txID = chdr.TxId
				invokeCC, upgradeCC, err := v.getTxCCInstance(payload)
				if err != nil {
					txsfltr.SetFlag(tIdx, peer.TxValidationCode_INVALID_OTHER_REASON)
					continue
				}
				txsChaincodeName = invokeCC
				if upgradeCC != nil {
					txsUpgradedChaincode = upgradeCC
				}
			} else if common.HeaderType(chdr.Type) == common.HeaderType_CONFIG {
				configEnvelope, err := configtx.UnmarshalConfigEnvelope(payload.Data)
				if err != nil {
					err = errors.WithMessage(err, "error unmarshalling config which passed initial validity checks")
					logger.Criticalf("%+v", err)
					return err
				}
				if err := v.Support.Apply(configEnvelope); err != nil {
					err = errors.WithMessage(err, "error validating config which passed initial validity checks")
					logger.Criticalf("%+v", err)
					return err
				}
				// logger.Debugf("config transaction received for chain %s", channel)
			} else {
				logger.Warningf("Unknown transaction type [%s] in block number [%d] transaction index [%d]",
					common.HeaderType(chdr.Type), block.Header.Number, tIdx)
				txsfltr.SetFlag(tIdx, peer.TxValidationCode_UNKNOWN_TX_TYPE)
				continue
			}
			if _, err := proto.Marshal(env); err != nil {
				logger.Warningf("Cannot marshal transaction: %s", err)
				txsfltr.SetFlag(tIdx, peer.TxValidationCode_MARSHAL_TX_ERROR)
				continue
			}
			txsfltr.SetFlag(tIdx, peer.TxValidationCode_VALID)
			if txsChaincodeName != nil {
				txsChaincodeNames[tIdx] = txsChaincodeName
			}
			if txsUpgradedChaincode != nil {
				txsUpgradedChaincodes[tIdx] = txsUpgradedChaincode
			}
			txidArray[tIdx] = txID
		} else {
			txsfltr.SetFlag(tIdx, peer.TxValidationCode_NIL_ENVELOPE)
			continue
		}
	}*/

	// if we operate with this capability, we mark invalid any transaction that has a txid
	// which is equal to that of a previous tx in this block
	// qiji: remove validate test
	/*if v.Support.Capabilities().ForbidDuplicateTXIdInBlock() {
		markTXIdDuplicates(txidArray, txsfltr)
	}*/

	// if we're here, all workers have completed validation and
	// no error was reported; we set the tx filter and return
	// success
	// qiji: remove test
	/* v.invalidTXsForUpgradeCC(txsChaincodeNames, txsUpgradedChaincodes, txsfltr)*/

	// // make sure no transaction has skipped validation
	// err = v.allValidated(txsfltr, block)
	// if err != nil {
	// 	return err
	// }

	// Initialize metadata structure
	utils.InitBlockMetadata(block)

	block.Metadata.Metadata[common.BlockMetadataIndex_TRANSACTIONS_FILTER] = txsfltr

	elapsedValidation := time.Since(startValidation) / time.Millisecond // duration in ms
	logger.Infof("[%s] Validated block [%d] in %dms", v.ChainID, block.Header.Number, elapsedValidation)

	return nil
}

// allValidated returns error if some of the validation flags have not been set
// during validation
func (v *TxValidator) allValidated(txsfltr ledgerUtil.TxValidationFlags, block *common.Block) error {
	for id, f := range txsfltr {
		if peer.TxValidationCode(f) == peer.TxValidationCode_NOT_VALIDATED {
			return errors.Errorf("transaction %d in block %d has skipped validation", id, block.Header.Number)
		}
	}

	return nil
}

func markTXIdDuplicates(txids []string, txsfltr ledgerUtil.TxValidationFlags) {
	txidMap := make(map[string]struct{})

	for id, txid := range txids {
		if txid == "" {
			continue
		}

		_, in := txidMap[txid]
		if in {
			logger.Error("Duplicate txid", txid, "found, skipping")
			txsfltr.SetFlag(id, peer.TxValidationCode_DUPLICATE_TXID)
		} else {
			txidMap[txid] = struct{}{}
		}
	}
}

func (v *TxValidator) validateTx(req *blockValidationRequest, results chan<- *blockValidationResult) {
	block := req.block
	d := req.d
	tIdx := req.tIdx
	txID := ""

	if d == nil {
		results <- &blockValidationResult{
			tIdx: tIdx,
		}
		return
	}

	if env, err := utils.GetEnvelopeFromBlock(d); err != nil {
		logger.Warningf("Error getting tx from block: %+v", err)
		results <- &blockValidationResult{
			tIdx:           tIdx,
			validationCode: peer.TxValidationCode_INVALID_OTHER_REASON,
		}
		return
	} else if env != nil {
		// validate the transaction: here we check that the transaction
		// is properly formed, properly signed and that the security
		// chain binding proposal to endorsements to tx holds. We do
		// NOT check the validity of endorsements, though. That's a
		// job for VSCC below
		logger.Debugf("[%s] validateTx starts for block %p env %p txn %d", v.ChainID, block, env, tIdx)
		defer logger.Debugf("[%s] validateTx completes for block %p env %p txn %d", v.ChainID, block, env, tIdx)
		var payload *common.Payload
		var err error
		var txsChaincodeName *sysccprovider.ChaincodeInstance
		var txsUpgradedChaincode *sysccprovider.ChaincodeInstance

		// fmt.Println("jyp: validate transaction, next step(core/common/validation/msgvalidation.go)")
		// if payload, txResult = validation.ValidateTransaction(env, v.Support.Capabilities()); txResult != peer.TxValidationCode_VALID {
		// 	logger.Errorf("Invalid transaction with index %d", tIdx)
		// 	results <- &blockValidationResult{
		// 		tIdx:           tIdx,
		// 		validationCode: txResult,
		// 	}
		// 	return
		// }
		payload, err = utils.GetPayload(env)

		chdr, err := utils.UnmarshalChannelHeader(payload.Header.ChannelHeader)
		if err != nil {
			logger.Warningf("Could not unmarshal channel header, err %s, skipping", err)
			results <- &blockValidationResult{
				tIdx:           tIdx,
				validationCode: peer.TxValidationCode_INVALID_OTHER_REASON,
			}
			return
		}

		channel := chdr.ChannelId
		logger.Debugf("Transaction is for channel %s", channel)

		if !v.chainExists(channel) {
			logger.Errorf("Dropping transaction for non-existent channel %s", channel)
			results <- &blockValidationResult{
				tIdx:           tIdx,
				validationCode: peer.TxValidationCode_TARGET_CHAIN_NOT_FOUND,
			}
			return
		}

		if common.HeaderType(chdr.Type) == common.HeaderType_ENDORSER_TRANSACTION {
			// Check duplicate transactions
			txID = chdr.TxId
			// // GetTransactionByID will return:
			// _, err := v.Support.Ledger().GetTransactionByID(txID)
			// // 1) err == nil => there is already a tx in the ledger with the supplied id
			// if err == nil {
			// 	logger.Error("Duplicate transaction found, ", txID, ", skipping")
			// 	results <- &blockValidationResult{
			// 		tIdx:           tIdx,
			// 		validationCode: peer.TxValidationCode_DUPLICATE_TXID,
			// 	}
			// 	return
			// }
			// // 2) err is not of type blkstorage.NotFoundInIndexErr => we could not verify whether a tx with the supplied id is in the ledger
			// if _, isNotFoundInIndexErrType := err.(ledger.NotFoundInIndexErr); !isNotFoundInIndexErrType {
			// 	logger.Errorf("Ledger failure while attempting to detect duplicate status for txid %s, err '%s'. Aborting", txID, err)
			// 	results <- &blockValidationResult{
			// 		tIdx: tIdx,
			// 		err:  err,
			// 	}
			// 	return
			// }
			// 3) err is of type blkstorage.NotFoundInIndexErr => there is no tx with the supplied id in the ledger

			// Validate tx with vscc and policy
			// _, ms_file, ms_line, _ := runtime.Caller(0)
			// fmt.Println("jyp: breakdown(vscc start)", ms_file, ms_line, time.Now().String()[:26])
			// logger.Debug("Validating transaction vscc tx validate")
			// err, cde := v.Vscc.VSCCValidateTx(tIdx, payload, d, block)
			// if err != nil {
			// 	logger.Errorf("VSCCValidateTx for transaction txId = %s returned error: %s", txID, err)
			// 	switch err.(type) {
			// 	case *commonerrors.VSCCExecutionFailureError:
			// 		results <- &blockValidationResult{
			// 			tIdx: tIdx,
			// 			err:  err,
			// 		}
			// 		return
			// 	case *commonerrors.VSCCInfoLookupFailureError:
			// 		results <- &blockValidationResult{
			// 			tIdx: tIdx,
			// 			err:  err,
			// 		}
			// 		return
			// 	default:
			// 		results <- &blockValidationResult{
			// 			tIdx:           tIdx,
			// 			validationCode: cde,
			// 		}
			// 		return
			// 	}
			// }
			// _, ms_file, ms_line, _ = runtime.Caller(0)
			// fmt.Println("jyp: breakdown(vscc end)", ms_file, ms_line, time.Now().String()[:26])

			invokeCC, upgradeCC, err := v.getTxCCInstance(payload)
			if err != nil {
				logger.Errorf("Get chaincode instance from transaction txId = %s returned error: %+v", txID, err)
				results <- &blockValidationResult{
					tIdx:           tIdx,
					validationCode: peer.TxValidationCode_INVALID_OTHER_REASON,
				}
				return
			}
			txsChaincodeName = invokeCC
			if upgradeCC != nil {
				logger.Infof("Find chaincode upgrade transaction for chaincode %s on channel %s with new version %s", upgradeCC.ChaincodeName, upgradeCC.ChainID, upgradeCC.ChaincodeVersion)
				txsUpgradedChaincode = upgradeCC
			}
		} else if common.HeaderType(chdr.Type) == common.HeaderType_CONFIG {
			configEnvelope, err := configtx.UnmarshalConfigEnvelope(payload.Data)
			if err != nil {
				err = errors.WithMessage(err, "error unmarshalling config which passed initial validity checks")
				logger.Criticalf("%+v", err)
				results <- &blockValidationResult{
					tIdx: tIdx,
					err:  err,
				}
				return
			}

			if err := v.Support.Apply(configEnvelope); err != nil {
				err = errors.WithMessage(err, "error validating config which passed initial validity checks")
				logger.Criticalf("%+v", err)
				results <- &blockValidationResult{
					tIdx: tIdx,
					err:  err,
				}
				return
			}
			logger.Debugf("config transaction received for chain %s", channel)
		} else {
			logger.Warningf("Unknown transaction type [%s] in block number [%d] transaction index [%d]",
				common.HeaderType(chdr.Type), block.Header.Number, tIdx)
			results <- &blockValidationResult{
				tIdx:           tIdx,
				validationCode: peer.TxValidationCode_UNKNOWN_TX_TYPE,
			}
			return
		}

		if _, err := proto.Marshal(env); err != nil {
			logger.Warningf("Cannot marshal transaction: %s", err)
			results <- &blockValidationResult{
				tIdx:           tIdx,
				validationCode: peer.TxValidationCode_MARSHAL_TX_ERROR,
			}
			return
		}
		// Succeeded to pass down here, transaction is valid
		results <- &blockValidationResult{
			tIdx:                 tIdx,
			txsChaincodeName:     txsChaincodeName,
			txsUpgradedChaincode: txsUpgradedChaincode,
			validationCode:       peer.TxValidationCode_VALID,
			txid:                 txID,
		}
		return
	} else {
		logger.Warning("Nil tx from block")
		results <- &blockValidationResult{
			tIdx:           tIdx,
			validationCode: peer.TxValidationCode_NIL_ENVELOPE,
		}
		return
	}
}

// generateCCKey generates a unique identifier for chaincode in specific channel
func (v *TxValidator) generateCCKey(ccName, chainID string) string {
	return fmt.Sprintf("%s/%s", ccName, chainID)
}

// invalidTXsForUpgradeCC invalid all txs that should be invalided because of chaincode upgrade txs
func (v *TxValidator) invalidTXsForUpgradeCC(txsChaincodeNames map[int]*sysccprovider.ChaincodeInstance, txsUpgradedChaincodes map[int]*sysccprovider.ChaincodeInstance, txsfltr ledgerUtil.TxValidationFlags) {
	if len(txsUpgradedChaincodes) == 0 {
		return
	}

	// Invalid former cc upgrade txs if there're two or more txs upgrade the same cc
	finalValidUpgradeTXs := make(map[string]int)
	upgradedChaincodes := make(map[string]*sysccprovider.ChaincodeInstance)
	for tIdx, cc := range txsUpgradedChaincodes {
		if cc == nil {
			continue
		}
		upgradedCCKey := v.generateCCKey(cc.ChaincodeName, cc.ChainID)

		if finalIdx, exist := finalValidUpgradeTXs[upgradedCCKey]; !exist {
			finalValidUpgradeTXs[upgradedCCKey] = tIdx
			upgradedChaincodes[upgradedCCKey] = cc
		} else if finalIdx < tIdx {
			logger.Infof("Invalid transaction with index %d: chaincode was upgraded by latter tx", finalIdx)
			txsfltr.SetFlag(finalIdx, peer.TxValidationCode_CHAINCODE_VERSION_CONFLICT)

			// record latter cc upgrade tx info
			finalValidUpgradeTXs[upgradedCCKey] = tIdx
			upgradedChaincodes[upgradedCCKey] = cc
		} else {
			logger.Infof("Invalid transaction with index %d: chaincode was upgraded by latter tx", tIdx)
			txsfltr.SetFlag(tIdx, peer.TxValidationCode_CHAINCODE_VERSION_CONFLICT)
		}
	}

	// invalid txs which invoke the upgraded chaincodes
	for tIdx, cc := range txsChaincodeNames {
		if cc == nil {
			continue
		}
		ccKey := v.generateCCKey(cc.ChaincodeName, cc.ChainID)
		if _, exist := upgradedChaincodes[ccKey]; exist {
			if txsfltr.IsValid(tIdx) {
				logger.Infof("Invalid transaction with index %d: chaincode was upgraded in the same block", tIdx)
				txsfltr.SetFlag(tIdx, peer.TxValidationCode_CHAINCODE_VERSION_CONFLICT)
			}
		}
	}
}

func (v *TxValidator) getTxCCInstance(payload *common.Payload) (invokeCCIns, upgradeCCIns *sysccprovider.ChaincodeInstance, err error) {
	// This is duplicated unpacking work, but make test easier.
	chdr, err := utils.UnmarshalChannelHeader(payload.Header.ChannelHeader)
	if err != nil {
		return nil, nil, err
	}

	// Chain ID
	chainID := chdr.ChannelId // it is guaranteed to be an existing channel by now

	// ChaincodeID
	hdrExt, err := utils.GetChaincodeHeaderExtension(payload.Header)
	if err != nil {
		return nil, nil, err
	}
	invokeCC := hdrExt.ChaincodeId
	invokeIns := &sysccprovider.ChaincodeInstance{ChainID: chainID, ChaincodeName: invokeCC.Name, ChaincodeVersion: invokeCC.Version}

	// Transaction
	tx, err := utils.GetTransaction(payload.Data)
	if err != nil {
		logger.Errorf("GetTransaction failed: %+v", err)
		return invokeIns, nil, nil
	}

	// ChaincodeActionPayload
	cap, err := utils.GetChaincodeActionPayload(tx.Actions[0].Payload)
	if err != nil {
		logger.Errorf("GetChaincodeActionPayload failed: %+v", err)
		return invokeIns, nil, nil
	}

	// ChaincodeProposalPayload
	cpp, err := utils.GetChaincodeProposalPayload(cap.ChaincodeProposalPayload)
	if err != nil {
		logger.Errorf("GetChaincodeProposalPayload failed: %+v", err)
		return invokeIns, nil, nil
	}

	// ChaincodeInvocationSpec
	cis := &peer.ChaincodeInvocationSpec{}
	err = proto.Unmarshal(cpp.Input, cis)
	if err != nil {
		logger.Errorf("GetChaincodeInvokeSpec failed: %+v", err)
		return invokeIns, nil, nil
	}

	if invokeCC.Name == "lscc" {
		if string(cis.ChaincodeSpec.Input.Args[0]) == "upgrade" {
			upgradeIns, err := v.getUpgradeTxInstance(chainID, cis.ChaincodeSpec.Input.Args[2])
			if err != nil {
				return invokeIns, nil, nil
			}
			return invokeIns, upgradeIns, nil
		}
	}

	return invokeIns, nil, nil
}

func (v *TxValidator) getUpgradeTxInstance(chainID string, cdsBytes []byte) (*sysccprovider.ChaincodeInstance, error) {
	cds, err := utils.GetChaincodeDeploymentSpec(cdsBytes, platforms.NewRegistry(&golang.Platform{}))
	if err != nil {
		return nil, err
	}

	return &sysccprovider.ChaincodeInstance{
		ChainID:          chainID,
		ChaincodeName:    cds.ChaincodeSpec.ChaincodeId.Name,
		ChaincodeVersion: cds.ChaincodeSpec.ChaincodeId.Version,
	}, nil
}

type dynamicDeserializer struct {
	support Support
}

func (ds *dynamicDeserializer) DeserializeIdentity(serializedIdentity []byte) (msp.Identity, error) {
	return ds.support.MSPManager().DeserializeIdentity(serializedIdentity)
}

func (ds *dynamicDeserializer) IsWellFormed(identity *mspprotos.SerializedIdentity) error {
	return ds.support.MSPManager().IsWellFormed(identity)
}

type dynamicCapabilities struct {
	support Support
}

func (ds *dynamicCapabilities) ACLs() bool {
	return ds.support.Capabilities().ACLs()
}

func (ds *dynamicCapabilities) CollectionUpgrade() bool {
	return ds.support.Capabilities().CollectionUpgrade()
}

func (ds *dynamicCapabilities) ForbidDuplicateTXIdInBlock() bool {
	return ds.support.Capabilities().ForbidDuplicateTXIdInBlock()
}

func (ds *dynamicCapabilities) KeyLevelEndorsement() bool {
	return ds.support.Capabilities().KeyLevelEndorsement()
}

func (ds *dynamicCapabilities) MetadataLifecycle() bool {
	return ds.support.Capabilities().MetadataLifecycle()
}

func (ds *dynamicCapabilities) PrivateChannelData() bool {
	return ds.support.Capabilities().PrivateChannelData()
}

func (ds *dynamicCapabilities) Supported() error {
	return ds.support.Capabilities().Supported()
}

func (ds *dynamicCapabilities) V1_1Validation() bool {
	return ds.support.Capabilities().V1_1Validation()
}

func (ds *dynamicCapabilities) V1_2Validation() bool {
	return ds.support.Capabilities().V1_2Validation()
}

func (ds *dynamicCapabilities) V1_3Validation() bool {
	return ds.support.Capabilities().V1_3Validation()
}

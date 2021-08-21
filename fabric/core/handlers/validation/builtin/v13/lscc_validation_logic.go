/*
Copyright IBM Corp. All Rights Reserved.

SPDX-License-Identifier: Apache-2.0
*/

package v13

import (
	"bytes"
	"fmt"

	"github.com/hyperledger/fabric/core/chaincode/platforms/ccmetadata"

	"github.com/golang/protobuf/proto"
	"github.com/hyperledger/fabric/common/channelconfig"
	commonerrors "github.com/hyperledger/fabric/common/errors"
	"github.com/hyperledger/fabric/core/chaincode/platforms"
	"github.com/hyperledger/fabric/core/chaincode/platforms/car"
	"github.com/hyperledger/fabric/core/chaincode/platforms/golang"
	"github.com/hyperledger/fabric/core/chaincode/platforms/java"
	"github.com/hyperledger/fabric/core/chaincode/platforms/node"
	"github.com/hyperledger/fabric/core/common/ccprovider"
	"github.com/hyperledger/fabric/core/common/privdata"
	. "github.com/hyperledger/fabric/core/handlers/validation/api/state"
	"github.com/hyperledger/fabric/core/ledger/kvledger/txmgmt/rwsetutil"
	"github.com/hyperledger/fabric/core/scc/lscc"
	"github.com/hyperledger/fabric/protos/common"
	"github.com/hyperledger/fabric/protos/ledger/rwset/kvrwset"
	pb "github.com/hyperledger/fabric/protos/peer"
	"github.com/hyperledger/fabric/protos/utils"
	"github.com/pkg/errors"
)

// checkInstantiationPolicy evaluates an instantiation policy against a signed proposal
func (vscc *Validator) checkInstantiationPolicy(chainName string, env *common.Envelope, instantiationPolicy []byte, payl *common.Payload) commonerrors.TxValidationError {
	// get the signature header
	shdr, err := utils.GetSignatureHeader(payl.Header.SignatureHeader)
	if err != nil {
		return policyErr(err)
	}

	// construct signed data we can evaluate the instantiation policy against
	sd := []*common.SignedData{{
		Data:      env.Payload,
		Identity:  shdr.Creator,
		Signature: env.Signature,
	}}
	err = vscc.policyEvaluator.Evaluate(instantiationPolicy, sd)
	if err != nil {
		return policyErr(fmt.Errorf("chaincode instantiation policy violated, error %s", err))
	}
	return nil
}

func validateNewCollectionConfigs(newCollectionConfigs []*common.CollectionConfig) error {
	newCollectionsMap := make(map[string]bool, len(newCollectionConfigs))
	// Process each collection config from a set of collection configs
	for _, newCollectionConfig := range newCollectionConfigs {

		newCollection := newCollectionConfig.GetStaticCollectionConfig()
		if newCollection == nil {
			return errors.New("unknown collection configuration type")
		}

		// Ensure that there are no duplicate collection names
		collectionName := newCollection.GetName()

		if err := validateCollectionName(collectionName); err != nil {
			return err
		}

		if _, ok := newCollectionsMap[collectionName]; !ok {
			newCollectionsMap[collectionName] = true
		} else {
			return fmt.Errorf("collection-name: %s -- found duplicate collection configuration", collectionName)
		}

		// Validate gossip related parameters present in the collection config
		maximumPeerCount := newCollection.GetMaximumPeerCount()
		requiredPeerCount := newCollection.GetRequiredPeerCount()
		if maximumPeerCount < requiredPeerCount {
			return fmt.Errorf("collection-name: %s -- maximum peer count (%d) cannot be greater than the required peer count (%d)",
				collectionName, maximumPeerCount, requiredPeerCount)

		}
		if requiredPeerCount < 0 {
			return fmt.Errorf("collection-name: %s -- requiredPeerCount (%d) cannot be less than zero (%d)",
				collectionName, maximumPeerCount, requiredPeerCount)

		}

		// make sure that the signature policy is meaningful (only consists of ORs)
		err := validateSpOrConcat(newCollection.MemberOrgsPolicy.GetSignaturePolicy().Rule)
		if err != nil {
			return errors.WithMessage(err, fmt.Sprintf("collection-name: %s -- error in member org policy", collectionName))
		}
	}
	return nil
}

// validateSpOrConcat checks if the supplied signature policy is just an OR-concatenation of identities
func validateSpOrConcat(sp *common.SignaturePolicy) error {
	if sp.GetNOutOf() == nil {
		return nil
	}
	// check if N == 1 (OR concatenation)
	if sp.GetNOutOf().N != 1 {
		return errors.New(fmt.Sprintf("signature policy is not an OR concatenation, NOutOf %d", sp.GetNOutOf().N))
	}
	// recurse into all sub-rules
	for _, rule := range sp.GetNOutOf().Rules {
		err := validateSpOrConcat(rule)
		if err != nil {
			return err
		}
	}
	return nil
}

func checkForMissingCollections(newCollectionsMap map[string]*common.StaticCollectionConfig, oldCollectionConfigs []*common.CollectionConfig,
) error {
	var missingCollections []string

	// In the new collection config package, ensure that there is one entry per old collection. Any
	// number of new collections are allowed.
	for _, oldCollectionConfig := range oldCollectionConfigs {

		oldCollection := oldCollectionConfig.GetStaticCollectionConfig()
		// It cannot be nil
		if oldCollection == nil {
			return policyErr(fmt.Errorf("unknown collection configuration type"))
		}

		// All old collection must exist in the new collection config package
		oldCollectionName := oldCollection.GetName()
		_, ok := newCollectionsMap[oldCollectionName]
		if !ok {
			missingCollections = append(missingCollections, oldCollectionName)
		}
	}

	if len(missingCollections) > 0 {
		return policyErr(fmt.Errorf("the following existing collections are missing in the new collection configuration package: %v",
			missingCollections))
	}

	return nil
}

func checkForModifiedCollectionsBTL(newCollectionsMap map[string]*common.StaticCollectionConfig, oldCollectionConfigs []*common.CollectionConfig,
) error {
	var modifiedCollectionsBTL []string

	// In the new collection config package, ensure that the block to live value is not
	// modified for the existing collections.
	for _, oldCollectionConfig := range oldCollectionConfigs {

		oldCollection := oldCollectionConfig.GetStaticCollectionConfig()
		// It cannot be nil
		if oldCollection == nil {
			return policyErr(fmt.Errorf("unknown collection configuration type"))
		}

		oldCollectionName := oldCollection.GetName()
		newCollection, _ := newCollectionsMap[oldCollectionName]
		// BlockToLive cannot be changed
		if newCollection.GetBlockToLive() != oldCollection.GetBlockToLive() {
			modifiedCollectionsBTL = append(modifiedCollectionsBTL, oldCollectionName)
		}
	}

	if len(modifiedCollectionsBTL) > 0 {
		return policyErr(fmt.Errorf("the BlockToLive in the following existing collections must not be modified: %v",
			modifiedCollectionsBTL))
	}

	return nil
}

func validateNewCollectionConfigsAgainstOld(newCollectionConfigs []*common.CollectionConfig, oldCollectionConfigs []*common.CollectionConfig,
) error {
	newCollectionsMap := make(map[string]*common.StaticCollectionConfig, len(newCollectionConfigs))

	for _, newCollectionConfig := range newCollectionConfigs {
		newCollection := newCollectionConfig.GetStaticCollectionConfig()
		// Collection object itself is stored as value so that we can
		// check whether the block to live is changed -- FAB-7810
		newCollectionsMap[newCollection.GetName()] = newCollection
	}

	if err := checkForMissingCollections(newCollectionsMap, oldCollectionConfigs); err != nil {
		return err
	}

	if err := checkForModifiedCollectionsBTL(newCollectionsMap, oldCollectionConfigs); err != nil {
		return err
	}

	return nil
}

func validateCollectionName(collectionName string) error {
	if collectionName == "" {
		return fmt.Errorf("empty collection-name is not allowed")
	}
	match := validCollectionNameRegex.FindString(collectionName)
	if len(match) != len(collectionName) {
		return fmt.Errorf("collection-name: %s not allowed. A valid collection name follows the pattern: %s",
			collectionName, ccmetadata.AllowedCharsCollectionName)
	}
	return nil
}

// validateRWSetAndCollection performs validation of the rwset
// of an LSCC deploy operation and then it validates any collection
// configuration
func (vscc *Validator) validateRWSetAndCollection(
	lsccrwset *kvrwset.KVRWSet,
	cdRWSet *ccprovider.ChaincodeData,
	lsccArgs [][]byte,
	lsccFunc string,
	ac channelconfig.ApplicationCapabilities,
	channelName string,
) commonerrors.TxValidationError {
	/********************************************/
	/* security check 0.a - validation of rwset */
	/********************************************/
	// there can only be one or two writes
	if len(lsccrwset.Writes) > 2 {
		return policyErr(fmt.Errorf("LSCC can only issue one or two putState upon deploy"))
	}

	/**********************************************************/
	/* security check 0.b - validation of the collection data */
	/**********************************************************/
	var collectionsConfigArg []byte
	if len(lsccArgs) > 5 {
		collectionsConfigArg = lsccArgs[5]
	}

	var collectionsConfigLedger []byte
	if len(lsccrwset.Writes) == 2 {
		key := privdata.BuildCollectionKVSKey(cdRWSet.Name)
		if lsccrwset.Writes[1].Key != key {
			return policyErr(fmt.Errorf("invalid key for the collection of chaincode %s:%s; expected '%s', received '%s'",
				cdRWSet.Name, cdRWSet.Version, key, lsccrwset.Writes[1].Key))

		}

		collectionsConfigLedger = lsccrwset.Writes[1].Value
	}

	if !bytes.Equal(collectionsConfigArg, collectionsConfigLedger) {
		return policyErr(fmt.Errorf("collection configuration arguments supplied for chaincode %s:%s do not match the configuration in the lscc writeset",
			cdRWSet.Name, cdRWSet.Version))

	}

	channelState, err := vscc.stateFetcher.FetchState()
	if err != nil {
		return &commonerrors.VSCCExecutionFailureError{Err: fmt.Errorf("failed obtaining query executor: %v", err)}
	}
	defer channelState.Done()

	state := &state{channelState}

	// The following condition check added in v1.1 may not be needed as it is not possible to have the chaincodeName~collection key in
	// the lscc namespace before a chaincode deploy. To avoid forks in v1.2, the following condition is retained.
	if lsccFunc == lscc.DEPLOY {
		colCriteria := common.CollectionCriteria{Channel: channelName, Namespace: cdRWSet.Name}
		ccp, err := privdata.RetrieveCollectionConfigPackageFromState(colCriteria, state)
		if err != nil {
			// fail if we get any error other than NoSuchCollectionError
			// because it means something went wrong while looking up the
			// older collection
			if _, ok := err.(privdata.NoSuchCollectionError); !ok {
				return &commonerrors.VSCCExecutionFailureError{Err: fmt.Errorf("unable to check whether collection existed earlier for chaincode %s:%s",
					cdRWSet.Name, cdRWSet.Version),
				}
			}
		}
		if ccp != nil {
			return policyErr(fmt.Errorf("collection data should not exist for chaincode %s:%s", cdRWSet.Name, cdRWSet.Version))
		}
	}

	// TODO: Once the new chaincode lifecycle is available (FAB-8724), the following validation
	// and other validation performed in ValidateLSCCInvocation can be moved to LSCC itself.
	newCollectionConfigPackage := &common.CollectionConfigPackage{}

	if collectionsConfigArg != nil {
		err := proto.Unmarshal(collectionsConfigArg, newCollectionConfigPackage)
		if err != nil {
			return policyErr(fmt.Errorf("invalid collection configuration supplied for chaincode %s:%s",
				cdRWSet.Name, cdRWSet.Version))
		}
	} else {
		return nil
	}

	if ac.V1_2Validation() {
		newCollectionConfigs := newCollectionConfigPackage.GetConfig()
		if err := validateNewCollectionConfigs(newCollectionConfigs); err != nil {
			return policyErr(err)
		}

		if lsccFunc == lscc.UPGRADE {

			collectionCriteria := common.CollectionCriteria{Channel: channelName, Namespace: cdRWSet.Name}
			// oldCollectionConfigPackage denotes the existing collection config package in the ledger
			oldCollectionConfigPackage, err := privdata.RetrieveCollectionConfigPackageFromState(collectionCriteria, state)
			if err != nil {
				// fail if we get any error other than NoSuchCollectionError
				// because it means something went wrong while looking up the
				// older collection
				if _, ok := err.(privdata.NoSuchCollectionError); !ok {
					return &commonerrors.VSCCExecutionFailureError{Err: fmt.Errorf("unable to check whether collection existed earlier for chaincode %s:%s: %v",
						cdRWSet.Name, cdRWSet.Version, err),
					}
				}
			}

			// oldCollectionConfigPackage denotes the existing collection config package in the ledger
			if oldCollectionConfigPackage != nil {
				oldCollectionConfigs := oldCollectionConfigPackage.GetConfig()
				if err := validateNewCollectionConfigsAgainstOld(newCollectionConfigs, oldCollectionConfigs); err != nil {
					return policyErr(err)
				}

			}
		}
	}

	return nil
}

func (vscc *Validator) ValidateLSCCInvocation(
	chid string,
	env *common.Envelope,
	cap *pb.ChaincodeActionPayload,
	payl *common.Payload,
	ac channelconfig.ApplicationCapabilities,
) commonerrors.TxValidationError {
	cpp, err := utils.GetChaincodeProposalPayload(cap.ChaincodeProposalPayload)
	if err != nil {
		logger.Errorf("VSCC error: GetChaincodeProposalPayload failed, err %s", err)
		return policyErr(err)
	}

	cis := &pb.ChaincodeInvocationSpec{}
	err = proto.Unmarshal(cpp.Input, cis)
	if err != nil {
		logger.Errorf("VSCC error: Unmarshal ChaincodeInvocationSpec failed, err %s", err)
		return policyErr(err)
	}

	if cis.ChaincodeSpec == nil ||
		cis.ChaincodeSpec.Input == nil ||
		cis.ChaincodeSpec.Input.Args == nil {
		logger.Errorf("VSCC error: committing invalid vscc invocation")
		return policyErr(fmt.Errorf("malformed chaincode invocation spec"))
	}

	lsccFunc := string(cis.ChaincodeSpec.Input.Args[0])
	lsccArgs := cis.ChaincodeSpec.Input.Args[1:]

	logger.Debugf("VSCC info: ValidateLSCCInvocation acting on %s %#v", lsccFunc, lsccArgs)

	switch lsccFunc {
	case lscc.UPGRADE, lscc.DEPLOY:
		logger.Debugf("VSCC info: validating invocation of lscc function %s on arguments %#v", lsccFunc, lsccArgs)

		if len(lsccArgs) < 2 {
			return policyErr(fmt.Errorf("Wrong number of arguments for invocation lscc(%s): expected at least 2, received %d", lsccFunc, len(lsccArgs)))
		}

		if (!ac.PrivateChannelData() && len(lsccArgs) > 5) ||
			(ac.PrivateChannelData() && len(lsccArgs) > 6) {
			return policyErr(fmt.Errorf("Wrong number of arguments for invocation lscc(%s): received %d", lsccFunc, len(lsccArgs)))
		}

		cdsArgs, err := utils.GetChaincodeDeploymentSpec(lsccArgs[1], platforms.NewRegistry(
			// XXX We should definitely _not_ have this external dependency in VSCC
			// as adding a platform could cause non-determinism.  This is yet another
			// reason why all of this custom LSCC validation at commit time has no
			// long term hope of staying deterministic and needs to be removed.
			&golang.Platform{},
			&node.Platform{},
			&java.Platform{},
			&car.Platform{},
		))

		if err != nil {
			return policyErr(fmt.Errorf("GetChaincodeDeploymentSpec error %s", err))
		}

		if cdsArgs == nil || cdsArgs.ChaincodeSpec == nil || cdsArgs.ChaincodeSpec.ChaincodeId == nil ||
			cap.Action == nil || cap.Action.ProposalResponsePayload == nil {
			return policyErr(fmt.Errorf("VSCC error: invocation of lscc(%s) does not have appropriate arguments", lsccFunc))
		}

		// get the rwset
		pRespPayload, err := utils.GetProposalResponsePayload(cap.Action.ProposalResponsePayload)
		if err != nil {
			return policyErr(fmt.Errorf("GetProposalResponsePayload error %s", err))
		}
		if pRespPayload.Extension == nil {
			return policyErr(fmt.Errorf("nil pRespPayload.Extension"))
		}
		respPayload, err := utils.GetChaincodeAction(pRespPayload.Extension)
		if err != nil {
			return policyErr(fmt.Errorf("GetChaincodeAction error %s", err))
		}
		txRWSet := &rwsetutil.TxRwSet{}
		if err = txRWSet.FromProtoBytes(respPayload.Results); err != nil {
			return policyErr(fmt.Errorf("txRWSet.FromProtoBytes error %s", err))
		}

		// extract the rwset for lscc
		var lsccrwset *kvrwset.KVRWSet
		for _, ns := range txRWSet.NsRwSets {
			logger.Debugf("Namespace %s", ns.NameSpace)
			if ns.NameSpace == "lscc" {
				lsccrwset = ns.KvRwSet
				break
			}
		}

		// retrieve from the ledger the entry for the chaincode at hand
		cdLedger, ccExistsOnLedger, err := vscc.getInstantiatedCC(chid, cdsArgs.ChaincodeSpec.ChaincodeId.Name)
		if err != nil {
			return &commonerrors.VSCCExecutionFailureError{Err: err}
		}

		/******************************************/
		/* security check 0 - validation of rwset */
		/******************************************/
		// there has to be a write-set
		if lsccrwset == nil {
			return policyErr(fmt.Errorf("No read write set for lscc was found"))
		}
		// there must be at least one write
		if len(lsccrwset.Writes) < 1 {
			return policyErr(fmt.Errorf("LSCC must issue at least one single putState upon deploy/upgrade"))
		}
		// the first key name must be the chaincode id provided in the deployment spec
		if lsccrwset.Writes[0].Key != cdsArgs.ChaincodeSpec.ChaincodeId.Name {
			return policyErr(fmt.Errorf("expected key %s, found %s", cdsArgs.ChaincodeSpec.ChaincodeId.Name, lsccrwset.Writes[0].Key))
		}
		// the value must be a ChaincodeData struct
		cdRWSet := &ccprovider.ChaincodeData{}
		err = proto.Unmarshal(lsccrwset.Writes[0].Value, cdRWSet)
		if err != nil {
			return policyErr(fmt.Errorf("unmarhsalling of ChaincodeData failed, error %s", err))
		}
		// the chaincode name in the lsccwriteset must match the chaincode name in the deployment spec
		if cdRWSet.Name != cdsArgs.ChaincodeSpec.ChaincodeId.Name {
			return policyErr(fmt.Errorf("expected cc name %s, found %s", cdsArgs.ChaincodeSpec.ChaincodeId.Name, cdRWSet.Name))
		}
		// the chaincode version in the lsccwriteset must match the chaincode version in the deployment spec
		if cdRWSet.Version != cdsArgs.ChaincodeSpec.ChaincodeId.Version {
			return policyErr(fmt.Errorf("expected cc version %s, found %s", cdsArgs.ChaincodeSpec.ChaincodeId.Version, cdRWSet.Version))
		}
		// it must only write to 2 namespaces: LSCC's and the cc that we are deploying/upgrading
		for _, ns := range txRWSet.NsRwSets {
			if ns.NameSpace != "lscc" && ns.NameSpace != cdRWSet.Name && len(ns.KvRwSet.Writes) > 0 {
				return policyErr(fmt.Errorf("LSCC invocation is attempting to write to namespace %s", ns.NameSpace))
			}
		}

		logger.Debugf("Validating %s for cc %s version %s", lsccFunc, cdRWSet.Name, cdRWSet.Version)

		switch lsccFunc {
		case lscc.DEPLOY:

			/******************************************************************/
			/* security check 1 - cc not in the LCCC table of instantiated cc */
			/******************************************************************/
			if ccExistsOnLedger {
				return policyErr(fmt.Errorf("Chaincode %s is already instantiated", cdsArgs.ChaincodeSpec.ChaincodeId.Name))
			}

			/****************************************************************************/
			/* security check 2 - validation of rwset (and of collections if enabled) */
			/****************************************************************************/
			if ac.PrivateChannelData() {
				// do extra validation for collections
				err := vscc.validateRWSetAndCollection(lsccrwset, cdRWSet, lsccArgs, lsccFunc, ac, chid)
				if err != nil {
					return err
				}
			} else {
				// there can only be a single ledger write
				if len(lsccrwset.Writes) != 1 {
					return policyErr(fmt.Errorf("LSCC can only issue a single putState upon deploy"))
				}
			}

			/*****************************************************/
			/* security check 3 - check the instantiation policy */
			/*****************************************************/
			pol := cdRWSet.InstantiationPolicy
			if pol == nil {
				return policyErr(fmt.Errorf("no instantiation policy was specified"))
			}
			// FIXME: could we actually pull the cds package from the
			// file system to verify whether the policy that is specified
			// here is the same as the one on disk?
			// PROS: we prevent attacks where the policy is replaced
			// CONS: this would be a point of non-determinism
			err := vscc.checkInstantiationPolicy(chid, env, pol, payl)
			if err != nil {
				return err
			}

		case lscc.UPGRADE:
			/**************************************************************/
			/* security check 1 - cc in the LCCC table of instantiated cc */
			/**************************************************************/
			if !ccExistsOnLedger {
				return policyErr(fmt.Errorf("Upgrading non-existent chaincode %s", cdsArgs.ChaincodeSpec.ChaincodeId.Name))
			}

			/**********************************************************/
			/* security check 2 - existing cc's version was different */
			/**********************************************************/
			if cdLedger.Version == cdsArgs.ChaincodeSpec.ChaincodeId.Version {
				return policyErr(fmt.Errorf("Existing version of the cc on the ledger (%s) should be different from the upgraded one", cdsArgs.ChaincodeSpec.ChaincodeId.Version))
			}

			/****************************************************************************/
			/* security check 3 validation of rwset (and of collections if enabled) */
			/****************************************************************************/
			// Only in v1.2, a collection can be updated during a chaincode upgrade
			if ac.V1_2Validation() {
				// do extra validation for collections
				err := vscc.validateRWSetAndCollection(lsccrwset, cdRWSet, lsccArgs, lsccFunc, ac, chid)
				if err != nil {
					return err
				}
			} else {
				// there can only be a single ledger write
				if len(lsccrwset.Writes) != 1 {
					return policyErr(fmt.Errorf("LSCC can only issue a single putState upon upgrade"))
				}
			}

			/*****************************************************/
			/* security check 4 - check the instantiation policy */
			/*****************************************************/
			pol := cdLedger.InstantiationPolicy
			if pol == nil {
				return policyErr(fmt.Errorf("No instantiation policy was specified"))
			}
			// FIXME: could we actually pull the cds package from the
			// file system to verify whether the policy that is specified
			// here is the same as the one on disk?
			// PROS: we prevent attacks where the policy is replaced
			// CONS: this would be a point of non-determinism
			err := vscc.checkInstantiationPolicy(chid, env, pol, payl)
			if err != nil {
				return err
			}

			/******************************************************************/
			/* security check 5 - check the instantiation policy in the rwset */
			/******************************************************************/
			if ac.V1_1Validation() {
				polNew := cdRWSet.InstantiationPolicy
				if polNew == nil {
					return policyErr(fmt.Errorf("No instantiation policy was specified"))
				}

				// no point in checking it again if they are the same policy
				if !bytes.Equal(polNew, pol) {
					err = vscc.checkInstantiationPolicy(chid, env, polNew, payl)
					if err != nil {
						return err
					}
				}
			}
		}

		// all is good!
		return nil
	default:
		return policyErr(fmt.Errorf("VSCC error: committing an invocation of function %s of lscc is invalid", lsccFunc))
	}
}

func (vscc *Validator) getInstantiatedCC(chid, ccid string) (cd *ccprovider.ChaincodeData, exists bool, err error) {
	qe, err := vscc.stateFetcher.FetchState()
	if err != nil {
		err = fmt.Errorf("could not retrieve QueryExecutor for channel %s, error %s", chid, err)
		return
	}
	defer qe.Done()
	channelState := &state{qe}
	bytes, err := channelState.GetState("lscc", ccid)
	if err != nil {
		err = fmt.Errorf("could not retrieve state for chaincode %s on channel %s, error %s", ccid, chid, err)
		return
	}

	if bytes == nil {
		return
	}

	cd = &ccprovider.ChaincodeData{}
	err = proto.Unmarshal(bytes, cd)
	if err != nil {
		err = fmt.Errorf("unmarshalling ChaincodeQueryResponse failed, error %s", err)
		return
	}

	exists = true
	return
}

type state struct {
	State
}

// GetState retrieves the value for the given key in the given namespace
func (s *state) GetState(namespace string, key string) ([]byte, error) {
	values, err := s.GetStateMultipleKeys(namespace, []string{key})
	if err != nil {
		return nil, err
	}
	if len(values) == 0 {
		return nil, nil
	}
	return values[0], nil
}

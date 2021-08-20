/*
Copyright IBM Corp. All Rights Reserved.

SPDX-License-Identifier: Apache-2.0
*/

package privdata

import (
	"strings"

	"github.com/hyperledger/fabric/protos/common"
)

// Collection defines a common interface for collections
type Collection interface {
	// SetTxContext configures the tx-specific ephemeral collection info, such
	// as txid, nonce, creator -- for future use
	// SetTxContext(parameters ...interface{})

	// CollectionID returns this collection's ID
	CollectionID() string

	// GetEndorsementPolicy returns the endorsement policy for validation -- for
	// future use
	// GetEndorsementPolicy() string

	// MemberOrgs returns the collection's members as MSP IDs. This serves as
	// a human-readable way of quickly identifying who is part of a collection.
	MemberOrgs() []string
}

// CollectionAccessPolicy encapsulates functions for the access policy of a collection
type CollectionAccessPolicy interface {
	// AccessFilter returns a member filter function for a collection
	AccessFilter() Filter

	// The minimum number of peers private data will be sent to upon
	// endorsement. The endorsement would fail if dissemination to at least
	// this number of peers is not achieved.
	RequiredPeerCount() int

	// The maximum number of peers that private data will be sent to
	// upon endorsement. This number has to be bigger than RequiredPeerCount().
	MaximumPeerCount() int

	// MemberOrgs returns the collection's members as MSP IDs. This serves as
	// a human-readable way of quickly identifying who is part of a collection.
	MemberOrgs() []string
}

// CollectionPersistenceConfigs encapsulates configurations related to persistece of a collection
type CollectionPersistenceConfigs interface {
	// BlockToLive returns the number of blocks after which the collection data expires.
	// For instance if the value is set to 10, a key last modified by block number 100
	// will be purged at block number 111. A zero value is treated same as MaxUint64
	BlockToLive() uint64
}

// Filter defines a rule that filters peers according to data signed by them.
// The Identity in the SignedData is a SerializedIdentity of a peer.
// The Data is a message the peer signed, and the Signature is the corresponding
// Signature on that Data.
// Returns: True, if the policy holds for the given signed data.
//          False otherwise
type Filter func(common.SignedData) bool

// CollectionStore retrieves stored collections based on the collection's
// properties. It works as a collection object factory and takes care of
// returning a collection object of an appropriate collection type.
type CollectionStore interface {
	// GetCollection retrieves the collection in the following way:
	// If the TxID exists in the ledger, the collection that is returned has the
	// latest configuration that was committed into the ledger before this txID
	// was committed.
	// Else - it's the latest configuration for the collection.
	RetrieveCollection(common.CollectionCriteria) (Collection, error)

	// GetCollectionAccessPolicy retrieves a collection's access policy
	RetrieveCollectionAccessPolicy(common.CollectionCriteria) (CollectionAccessPolicy, error)

	// RetrieveCollectionConfigPackage retrieves the whole configuration package
	// for the chaincode with the supplied criteria
	RetrieveCollectionConfigPackage(common.CollectionCriteria) (*common.CollectionConfigPackage, error)

	// RetrieveCollectionPersistenceConfigs retrieves the collection's persistence related configurations
	RetrieveCollectionPersistenceConfigs(cc common.CollectionCriteria) (CollectionPersistenceConfigs, error)

	CollectionFilter
}

type CollectionFilter interface {
	// AccessFilter retrieves the collection's filter that matches a given channel and a collectionPolicyConfig
	AccessFilter(channelName string, collectionPolicyConfig *common.CollectionPolicyConfig) (Filter, error)
}

const (
	// Collecion-specific constants

	// CollectionSeparator is the separator used to build the KVS
	// key storing the collections of a chaincode; note that we are
	// using as separator a character which is illegal for either the
	// name or the version of a chaincode so there cannot be any
	// collisions when chosing the name
	collectionSeparator = "~"
	// collectionSuffix is the suffix of the KVS key storing the
	// collections of a chaincode
	collectionSuffix = "collection"
)

// BuildCollectionKVSKey constructs the collection config key for a given chaincode name
func BuildCollectionKVSKey(ccname string) string {
	return ccname + collectionSeparator + collectionSuffix
}

// IsCollectionConfigKey detects if a key is a collection key
func IsCollectionConfigKey(key string) bool {
	return strings.Contains(key, collectionSeparator)
}

// GetCCNameFromCollectionConfigKey returns the chaincode name given a collection config key
func GetCCNameFromCollectionConfigKey(key string) string {
	splittedKey := strings.Split(key, collectionSeparator)
	return splittedKey[0]
}

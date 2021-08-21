/*
Copyright IBM Corp. All Rights Reserved.

SPDX-License-Identifier: Apache-2.0
*/

package privdata

import (
	"errors"
	"testing"

	"github.com/hyperledger/fabric/core/ledger"
	"github.com/hyperledger/fabric/core/transientstore"
	privdatacommon "github.com/hyperledger/fabric/gossip/privdata/common"
	"github.com/hyperledger/fabric/gossip/privdata/mocks"
	"github.com/hyperledger/fabric/protos/common"
	gossip2 "github.com/hyperledger/fabric/protos/gossip"
	"github.com/hyperledger/fabric/protos/ledger/rwset"
	transientstore2 "github.com/hyperledger/fabric/protos/transientstore"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/mock"
)

/*
	Test checks following scenario, it tries to obtain private data for
	given block sequence which is greater than available ledger height,
	hence data should be looked up directly from transient store
*/
func TestNewDataRetriever_GetDataFromTransientStore(t *testing.T) {
	t.Parallel()
	dataStore := &mocks.DataStore{}

	rwSetScanner := &mocks.RWSetScanner{}
	namespace := "testChaincodeName1"
	collectionName := "testCollectionName"

	rwSetScanner.On("Close")
	rwSetScanner.On("NextWithConfig").Return(&transientstore.EndorserPvtSimulationResultsWithConfig{
		ReceivedAtBlockHeight:          2,
		PvtSimulationResultsWithConfig: nil,
	}, nil).Once().On("NextWithConfig").Return(&transientstore.EndorserPvtSimulationResultsWithConfig{
		ReceivedAtBlockHeight: 2,
		PvtSimulationResultsWithConfig: &transientstore2.TxPvtReadWriteSetWithConfigInfo{
			PvtRwset: &rwset.TxPvtReadWriteSet{
				DataModel: rwset.TxReadWriteSet_KV,
				NsPvtRwset: []*rwset.NsPvtReadWriteSet{
					pvtReadWriteSet(namespace, collectionName, []byte{1, 2}),
					pvtReadWriteSet(namespace, collectionName, []byte{3, 4}),
				},
			},
			CollectionConfigs: map[string]*common.CollectionConfigPackage{
				namespace: {
					Config: []*common.CollectionConfig{
						{
							Payload: &common.CollectionConfig_StaticCollectionConfig{
								StaticCollectionConfig: &common.StaticCollectionConfig{
									Name: collectionName,
								},
							},
						},
					},
				},
			},
		},
	}, nil).
		Once(). // return only once results, next call should return and empty result
		On("NextWithConfig").Return(nil, nil)

	dataStore.On("LedgerHeight").Return(uint64(1), nil)
	dataStore.On("GetTxPvtRWSetByTxid", "testTxID", mock.Anything).Return(rwSetScanner, nil)

	retriever := NewDataRetriever(dataStore)

	// Request digest for private data which is greater than current ledger height
	// to make it query transient store for missed private data
	rwSets, err := retriever.CollectionRWSet([]*gossip2.PvtDataDigest{{
		Namespace:  namespace,
		Collection: collectionName,
		BlockSeq:   2,
		TxId:       "testTxID",
		SeqInBlock: 1,
	}}, 2)

	assertion := assert.New(t)
	assertion.NoError(err)
	assertion.NotEmpty(rwSets)
	dig2pvtRWSet := rwSets[privdatacommon.DigKey{
		Namespace:  namespace,
		Collection: collectionName,
		BlockSeq:   2,
		TxId:       "testTxID",
		SeqInBlock: 1,
	}]
	assertion.NotNil(dig2pvtRWSet)
	pvtRWSets := dig2pvtRWSet.RWSet
	assertion.Equal(2, len(pvtRWSets))

	var mergedRWSet []byte
	for _, rws := range pvtRWSets {
		mergedRWSet = append(mergedRWSet, rws...)
	}

	assertion.Equal([]byte{1, 2, 3, 4}, mergedRWSet)
}

/*
	Simple test case where available ledger height is greater than
	requested block sequence and therefore private data will be retrieved
	from the ledger rather than transient store as data being committed
*/
func TestNewDataRetriever_GetDataFromLedger(t *testing.T) {
	t.Parallel()
	dataStore := &mocks.DataStore{}

	namespace := "testChaincodeName1"
	collectionName := "testCollectionName"

	result := []*ledger.TxPvtData{{
		WriteSet: &rwset.TxPvtReadWriteSet{
			DataModel: rwset.TxReadWriteSet_KV,
			NsPvtRwset: []*rwset.NsPvtReadWriteSet{
				pvtReadWriteSet(namespace, collectionName, []byte{1, 2}),
				pvtReadWriteSet(namespace, collectionName, []byte{3, 4}),
			},
		},
		SeqInBlock: 1,
	}}

	dataStore.On("LedgerHeight").Return(uint64(10), nil)
	dataStore.On("GetPvtDataByNum", uint64(5), mock.Anything).Return(result, nil)

	historyRetreiver := &mocks.ConfigHistoryRetriever{}
	historyRetreiver.On("MostRecentCollectionConfigBelow", mock.Anything, namespace).Return(newCollectionConfig(collectionName), nil)
	dataStore.On("GetConfigHistoryRetriever").Return(historyRetreiver, nil)

	retriever := NewDataRetriever(dataStore)

	// Request digest for private data which is greater than current ledger height
	// to make it query ledger for missed private data
	rwSets, err := retriever.CollectionRWSet([]*gossip2.PvtDataDigest{{
		Namespace:  namespace,
		Collection: collectionName,
		BlockSeq:   uint64(5),
		TxId:       "testTxID",
		SeqInBlock: 1,
	}}, uint64(5))

	assertion := assert.New(t)
	assertion.NoError(err)
	assertion.NotEmpty(rwSets)
	pvtRWSet := rwSets[privdatacommon.DigKey{
		Namespace:  namespace,
		Collection: collectionName,
		BlockSeq:   5,
		TxId:       "testTxID",
		SeqInBlock: 1,
	}]
	assertion.NotEmpty(pvtRWSet)
	assertion.Equal(2, len(pvtRWSet.RWSet))

	var mergedRWSet []byte
	for _, rws := range pvtRWSet.RWSet {
		mergedRWSet = append(mergedRWSet, rws...)
	}

	assertion.Equal([]byte{1, 2, 3, 4}, mergedRWSet)
}

func TestNewDataRetriever_FailGetPvtDataFromLedger(t *testing.T) {
	t.Parallel()
	dataStore := &mocks.DataStore{}

	namespace := "testChaincodeName1"
	collectionName := "testCollectionName"

	dataStore.On("LedgerHeight").Return(uint64(10), nil)
	dataStore.On("GetPvtDataByNum", uint64(5), mock.Anything).
		Return(nil, errors.New("failing retrieving private data"))

	retriever := NewDataRetriever(dataStore)

	// Request digest for private data which is greater than current ledger height
	// to make it query transient store for missed private data
	rwSets, err := retriever.CollectionRWSet([]*gossip2.PvtDataDigest{{
		Namespace:  namespace,
		Collection: collectionName,
		BlockSeq:   uint64(5),
		TxId:       "testTxID",
		SeqInBlock: 1,
	}}, uint64(5))

	assertion := assert.New(t)
	assertion.Error(err)
	assertion.Empty(rwSets)
}

func TestNewDataRetriever_GetOnlyRelevantPvtData(t *testing.T) {
	t.Parallel()
	dataStore := &mocks.DataStore{}

	namespace := "testChaincodeName1"
	collectionName := "testCollectionName"

	result := []*ledger.TxPvtData{{
		WriteSet: &rwset.TxPvtReadWriteSet{
			DataModel: rwset.TxReadWriteSet_KV,
			NsPvtRwset: []*rwset.NsPvtReadWriteSet{
				pvtReadWriteSet(namespace, collectionName, []byte{1}),
				pvtReadWriteSet(namespace, collectionName, []byte{2}),
				pvtReadWriteSet("invalidNamespace", collectionName, []byte{0, 0}),
				pvtReadWriteSet(namespace, "invalidCollectionName", []byte{0, 0}),
			},
		},
		SeqInBlock: 1,
	}}

	dataStore.On("LedgerHeight").Return(uint64(10), nil)
	dataStore.On("GetPvtDataByNum", uint64(5), mock.Anything).Return(result, nil)
	historyRetreiver := &mocks.ConfigHistoryRetriever{}
	historyRetreiver.On("MostRecentCollectionConfigBelow", mock.Anything, namespace).Return(newCollectionConfig(collectionName), nil)
	dataStore.On("GetConfigHistoryRetriever").Return(historyRetreiver, nil)

	retriever := NewDataRetriever(dataStore)

	// Request digest for private data which is greater than current ledger height
	// to make it query transient store for missed private data
	rwSets, err := retriever.CollectionRWSet([]*gossip2.PvtDataDigest{{
		Namespace:  namespace,
		Collection: collectionName,
		BlockSeq:   uint64(5),
		TxId:       "testTxID",
		SeqInBlock: 1,
	}}, 5)

	assertion := assert.New(t)
	assertion.NoError(err)
	assertion.NotEmpty(rwSets)
	pvtRWSet := rwSets[privdatacommon.DigKey{
		Namespace:  namespace,
		Collection: collectionName,
		BlockSeq:   5,
		TxId:       "testTxID",
		SeqInBlock: 1,
	}]
	assertion.NotEmpty(pvtRWSet)
	assertion.Equal(2, len(pvtRWSet.RWSet))

	var mergedRWSet []byte
	for _, rws := range pvtRWSet.RWSet {
		mergedRWSet = append(mergedRWSet, rws...)
	}

	assertion.Equal([]byte{1, 2}, mergedRWSet)
}

func TestNewDataRetriever_GetMultipleDigests(t *testing.T) {
	t.Parallel()
	dataStore := &mocks.DataStore{}

	ns1, ns2 := "testChaincodeName1", "testChaincodeName2"
	col1, col2 := "testCollectionName1", "testCollectionName2"

	result := []*ledger.TxPvtData{
		{
			WriteSet: &rwset.TxPvtReadWriteSet{
				DataModel: rwset.TxReadWriteSet_KV,
				NsPvtRwset: []*rwset.NsPvtReadWriteSet{
					pvtReadWriteSet(ns1, col1, []byte{1}),
					pvtReadWriteSet(ns1, col1, []byte{2}),
					pvtReadWriteSet("invalidNamespace", col1, []byte{0, 0}),
					pvtReadWriteSet(ns1, "invalidCollectionName", []byte{0, 0}),
				},
			},
			SeqInBlock: 1,
		},
		{
			WriteSet: &rwset.TxPvtReadWriteSet{
				DataModel: rwset.TxReadWriteSet_KV,
				NsPvtRwset: []*rwset.NsPvtReadWriteSet{
					pvtReadWriteSet(ns2, col2, []byte{3}),
					pvtReadWriteSet(ns2, col2, []byte{4}),
					pvtReadWriteSet("invalidNamespace", col2, []byte{0, 0}),
					pvtReadWriteSet(ns2, "invalidCollectionName", []byte{0, 0}),
				},
			},
			SeqInBlock: 2,
		},
		{
			WriteSet: &rwset.TxPvtReadWriteSet{
				DataModel: rwset.TxReadWriteSet_KV,
				NsPvtRwset: []*rwset.NsPvtReadWriteSet{
					pvtReadWriteSet(ns1, col1, []byte{5}),
					pvtReadWriteSet(ns2, col2, []byte{6}),
					pvtReadWriteSet("invalidNamespace", col2, []byte{0, 0}),
					pvtReadWriteSet(ns2, "invalidCollectionName", []byte{0, 0}),
				},
			},
			SeqInBlock: 3,
		},
	}

	dataStore.On("LedgerHeight").Return(uint64(10), nil)
	dataStore.On("GetPvtDataByNum", uint64(5), mock.Anything).Return(result, nil)
	historyRetreiver := &mocks.ConfigHistoryRetriever{}
	historyRetreiver.On("MostRecentCollectionConfigBelow", mock.Anything, ns1).Return(newCollectionConfig(col1), nil)
	historyRetreiver.On("MostRecentCollectionConfigBelow", mock.Anything, ns2).Return(newCollectionConfig(col2), nil)
	dataStore.On("GetConfigHistoryRetriever").Return(historyRetreiver, nil)

	retriever := NewDataRetriever(dataStore)

	// Request digest for private data which is greater than current ledger height
	// to make it query transient store for missed private data
	rwSets, err := retriever.CollectionRWSet([]*gossip2.PvtDataDigest{{
		Namespace:  ns1,
		Collection: col1,
		BlockSeq:   uint64(5),
		TxId:       "testTxID",
		SeqInBlock: 1,
	}, {
		Namespace:  ns2,
		Collection: col2,
		BlockSeq:   uint64(5),
		TxId:       "testTxID",
		SeqInBlock: 2,
	}}, 5)

	assertion := assert.New(t)
	assertion.NoError(err)
	assertion.NotEmpty(rwSets)
	assertion.Equal(2, len(rwSets))

	pvtRWSet := rwSets[privdatacommon.DigKey{
		Namespace:  ns1,
		Collection: col1,
		BlockSeq:   5,
		TxId:       "testTxID",
		SeqInBlock: 1,
	}]
	assertion.NotEmpty(pvtRWSet)
	assertion.Equal(2, len(pvtRWSet.RWSet))

	var mergedRWSet []byte
	for _, rws := range pvtRWSet.RWSet {
		mergedRWSet = append(mergedRWSet, rws...)
	}

	pvtRWSet = rwSets[privdatacommon.DigKey{
		Namespace:  ns2,
		Collection: col2,
		BlockSeq:   5,
		TxId:       "testTxID",
		SeqInBlock: 2,
	}]
	assertion.NotEmpty(pvtRWSet)
	assertion.Equal(2, len(pvtRWSet.RWSet))
	for _, rws := range pvtRWSet.RWSet {
		mergedRWSet = append(mergedRWSet, rws...)
	}

	assertion.Equal([]byte{1, 2, 3, 4}, mergedRWSet)
}

func TestNewDataRetriever_EmptyWriteSet(t *testing.T) {
	t.Parallel()
	dataStore := &mocks.DataStore{}

	ns1 := "testChaincodeName1"
	col1 := "testCollectionName1"

	result := []*ledger.TxPvtData{
		{
			SeqInBlock: 1,
		},
	}

	dataStore.On("LedgerHeight").Return(uint64(10), nil)
	dataStore.On("GetPvtDataByNum", uint64(5), mock.Anything).Return(result, nil)
	historyRetreiver := &mocks.ConfigHistoryRetriever{}
	historyRetreiver.On("MostRecentCollectionConfigBelow", mock.Anything, ns1).Return(newCollectionConfig(col1), nil)
	dataStore.On("GetConfigHistoryRetriever").Return(historyRetreiver, nil)

	retriever := NewDataRetriever(dataStore)

	rwSets, err := retriever.CollectionRWSet([]*gossip2.PvtDataDigest{{
		Namespace:  ns1,
		Collection: col1,
		BlockSeq:   uint64(5),
		TxId:       "testTxID",
		SeqInBlock: 1,
	}}, 5)

	assertion := assert.New(t)
	assertion.NoError(err)
	assertion.NotEmpty(rwSets)

	pvtRWSet := rwSets[privdatacommon.DigKey{
		Namespace:  ns1,
		Collection: col1,
		BlockSeq:   5,
		TxId:       "testTxID",
		SeqInBlock: 1,
	}]
	assertion.NotEmpty(pvtRWSet)
	assertion.Empty(pvtRWSet.RWSet)

}

func TestNewDataRetriever_FailedObtainConfigHistoryRetriever(t *testing.T) {
	t.Parallel()
	dataStore := &mocks.DataStore{}

	ns1 := "testChaincodeName1"
	col1 := "testCollectionName1"

	result := []*ledger.TxPvtData{
		{
			WriteSet: &rwset.TxPvtReadWriteSet{
				DataModel: rwset.TxReadWriteSet_KV,
				NsPvtRwset: []*rwset.NsPvtReadWriteSet{
					pvtReadWriteSet(ns1, col1, []byte{1}),
					pvtReadWriteSet(ns1, col1, []byte{2}),
				},
			},
			SeqInBlock: 1,
		},
	}

	dataStore.On("LedgerHeight").Return(uint64(10), nil)
	dataStore.On("GetPvtDataByNum", uint64(5), mock.Anything).Return(result, nil)
	dataStore.On("GetConfigHistoryRetriever").Return(nil, errors.New("failed to obtain ConfigHistoryRetriever"))

	retriever := NewDataRetriever(dataStore)

	_, err := retriever.CollectionRWSet([]*gossip2.PvtDataDigest{{
		Namespace:  ns1,
		Collection: col1,
		BlockSeq:   uint64(5),
		TxId:       "testTxID",
		SeqInBlock: 1,
	}}, 5)

	assertion := assert.New(t)
	assertion.Contains(err.Error(), "failed to obtain ConfigHistoryRetriever")
}

func TestNewDataRetriever_NoCollectionConfig(t *testing.T) {
	t.Parallel()
	dataStore := &mocks.DataStore{}

	ns1, ns2 := "testChaincodeName1", "testChaincodeName2"
	col1, col2 := "testCollectionName1", "testCollectionName2"

	result := []*ledger.TxPvtData{
		{
			WriteSet: &rwset.TxPvtReadWriteSet{
				DataModel: rwset.TxReadWriteSet_KV,
				NsPvtRwset: []*rwset.NsPvtReadWriteSet{
					pvtReadWriteSet(ns1, col1, []byte{1}),
					pvtReadWriteSet(ns1, col1, []byte{2}),
				},
			},
			SeqInBlock: 1,
		},
		{
			WriteSet: &rwset.TxPvtReadWriteSet{
				DataModel: rwset.TxReadWriteSet_KV,
				NsPvtRwset: []*rwset.NsPvtReadWriteSet{
					pvtReadWriteSet(ns2, col2, []byte{3}),
					pvtReadWriteSet(ns2, col2, []byte{4}),
				},
			},
			SeqInBlock: 2,
		},
	}

	dataStore.On("LedgerHeight").Return(uint64(10), nil)
	dataStore.On("GetPvtDataByNum", uint64(5), mock.Anything).Return(result, nil)
	historyRetreiver := &mocks.ConfigHistoryRetriever{}
	historyRetreiver.On("MostRecentCollectionConfigBelow", mock.Anything, ns1).
		Return(newCollectionConfig(col1), errors.New("failed to obtain collection config"))
	historyRetreiver.On("MostRecentCollectionConfigBelow", mock.Anything, ns2).
		Return(nil, nil)
	dataStore.On("GetConfigHistoryRetriever").Return(historyRetreiver, nil)

	retriever := NewDataRetriever(dataStore)
	assertion := assert.New(t)

	_, err := retriever.CollectionRWSet([]*gossip2.PvtDataDigest{{
		Namespace:  ns1,
		Collection: col1,
		BlockSeq:   uint64(5),
		TxId:       "testTxID",
		SeqInBlock: 1,
	}}, 5)
	assertion.Error(err)
	assertion.Contains(err.Error(), "cannot find recent collection config update below block sequence")

	_, err = retriever.CollectionRWSet([]*gossip2.PvtDataDigest{{
		Namespace:  ns2,
		Collection: col2,
		BlockSeq:   uint64(5),
		TxId:       "testTxID",
		SeqInBlock: 2,
	}}, 5)
	assertion.Error(err)
	assertion.Contains(err.Error(), "no collection config update below block sequence")
}

func TestNewDataRetriever_FailedGetLedgerHeight(t *testing.T) {
	t.Parallel()
	dataStore := &mocks.DataStore{}

	ns1 := "testChaincodeName1"
	col1 := "testCollectionName1"

	dataStore.On("LedgerHeight").Return(uint64(0), errors.New("failed to read ledger height"))
	retriever := NewDataRetriever(dataStore)

	_, err := retriever.CollectionRWSet([]*gossip2.PvtDataDigest{{
		Namespace:  ns1,
		Collection: col1,
		BlockSeq:   uint64(5),
		TxId:       "testTxID",
		SeqInBlock: 1,
	}}, 5)

	assertion := assert.New(t)
	assertion.Error(err)
	assertion.Contains(err.Error(), "failed to read ledger height")
}

func TestNewDataRetriever_FailToReadFromTransientStore(t *testing.T) {
	t.Parallel()
	dataStore := &mocks.DataStore{}

	namespace := "testChaincodeName1"
	collectionName := "testCollectionName"

	dataStore.On("LedgerHeight").Return(uint64(1), nil)
	dataStore.On("GetTxPvtRWSetByTxid", "testTxID", mock.Anything).
		Return(nil, errors.New("fail to read form transient store"))

	retriever := NewDataRetriever(dataStore)

	rwset, err := retriever.CollectionRWSet([]*gossip2.PvtDataDigest{{
		Namespace:  namespace,
		Collection: collectionName,
		BlockSeq:   2,
		TxId:       "testTxID",
		SeqInBlock: 1,
	}}, 2)

	assertion := assert.New(t)
	assertion.NoError(err)
	assertion.Empty(rwset)
}

func TestNewDataRetriever_FailedToReadNext(t *testing.T) {
	t.Parallel()
	dataStore := &mocks.DataStore{}

	rwSetScanner := &mocks.RWSetScanner{}
	namespace := "testChaincodeName1"
	collectionName := "testCollectionName"

	rwSetScanner.On("Close")
	rwSetScanner.On("NextWithConfig").Return(nil, errors.New("failed to read next"))

	dataStore.On("LedgerHeight").Return(uint64(1), nil)
	dataStore.On("GetTxPvtRWSetByTxid", "testTxID", mock.Anything).Return(rwSetScanner, nil)

	retriever := NewDataRetriever(dataStore)

	// Request digest for private data which is greater than current ledger height
	// to make it query transient store for missed private data
	rwSets, err := retriever.CollectionRWSet([]*gossip2.PvtDataDigest{{
		Namespace:  namespace,
		Collection: collectionName,
		BlockSeq:   2,
		TxId:       "testTxID",
		SeqInBlock: 1,
	}}, 2)

	assertion := assert.New(t)
	assertion.NoError(err)
	assertion.Empty(rwSets)
}

func TestNewDataRetriever_EmptyPvtRWSetInTransientStore(t *testing.T) {
	t.Parallel()
	dataStore := &mocks.DataStore{}

	rwSetScanner := &mocks.RWSetScanner{}
	namespace := "testChaincodeName1"
	collectionName := "testCollectionName"

	rwSetScanner.On("Close")
	rwSetScanner.On("NextWithConfig").Return(&transientstore.EndorserPvtSimulationResultsWithConfig{
		ReceivedAtBlockHeight: 2,
		PvtSimulationResultsWithConfig: &transientstore2.TxPvtReadWriteSetWithConfigInfo{
			CollectionConfigs: map[string]*common.CollectionConfigPackage{
				namespace: {
					Config: []*common.CollectionConfig{
						{
							Payload: &common.CollectionConfig_StaticCollectionConfig{
								StaticCollectionConfig: &common.StaticCollectionConfig{
									Name: collectionName,
								},
							},
						},
					},
				},
			},
		},
	}, nil).
		Once(). // return only once results, next call should return and empty result
		On("NextWithConfig").Return(nil, nil)

	dataStore.On("LedgerHeight").Return(uint64(1), nil)
	dataStore.On("GetTxPvtRWSetByTxid", "testTxID", mock.Anything).Return(rwSetScanner, nil)

	retriever := NewDataRetriever(dataStore)

	rwSets, err := retriever.CollectionRWSet([]*gossip2.PvtDataDigest{{
		Namespace:  namespace,
		Collection: collectionName,
		BlockSeq:   2,
		TxId:       "testTxID",
		SeqInBlock: 1,
	}}, 2)

	assertion := assert.New(t)
	assertion.NoError(err)
	assertion.NotEmpty(rwSets)
	assertion.Empty(rwSets[privdatacommon.DigKey{
		Namespace:  namespace,
		Collection: collectionName,
		BlockSeq:   2,
		TxId:       "testTxID",
		SeqInBlock: 1,
	}])
}

func newCollectionConfig(collectionName string) *ledger.CollectionConfigInfo {
	return &ledger.CollectionConfigInfo{
		CollectionConfig: &common.CollectionConfigPackage{
			Config: []*common.CollectionConfig{
				{
					Payload: &common.CollectionConfig_StaticCollectionConfig{
						StaticCollectionConfig: &common.StaticCollectionConfig{
							Name: collectionName,
						},
					},
				},
			},
		},
	}
}

func pvtReadWriteSet(ns string, collectionName string, data []byte) *rwset.NsPvtReadWriteSet {
	return &rwset.NsPvtReadWriteSet{
		Namespace: ns,
		CollectionPvtRwset: []*rwset.CollectionPvtReadWriteSet{{
			CollectionName: collectionName,
			Rwset:          data,
		}},
	}
}

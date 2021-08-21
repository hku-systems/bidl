/*
Copyright IBM Corp. All Rights Reserved.

SPDX-License-Identifier: Apache-2.0
*/

package nwo_test

import (
	"encoding/json"
	"testing"

	"github.com/hyperledger/fabric/integration/nwo"
	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"
)

func TestNewWorldOrder(t *testing.T) {
	RegisterFailHandler(Fail)
	RunSpecs(t, "New World Order Suite")
}

var components *nwo.Components

var _ = SynchronizedBeforeSuite(func() []byte {
	components = &nwo.Components{}
	components.Build()

	payload, err := json.Marshal(components)
	Expect(err).NotTo(HaveOccurred())

	return payload
}, func(payload []byte) {
	err := json.Unmarshal(payload, &components)
	Expect(err).NotTo(HaveOccurred())
})

var _ = SynchronizedAfterSuite(func() {
}, func() {
	components.Cleanup()
})

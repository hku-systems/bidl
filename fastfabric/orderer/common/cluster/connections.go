/*
Copyright IBM Corp. 2017 All Rights Reserved.

SPDX-License-Identifier: Apache-2.0
*/

package cluster

import (
	"bytes"
	"crypto/x509"
	"sync"
	"sync/atomic"

	"github.com/pkg/errors"
	"google.golang.org/grpc"
)

// RemoteVerifier verifies the connection to the remote host
type RemoteVerifier func(rawCerts [][]byte, verifiedChains [][]*x509.Certificate) error

//go:generate mockery -dir . -name SecureDialer -case underscore -output ./mocks/

// SecureDialer connects to a remote address
type SecureDialer interface {
	Dial(address string, verifyFunc RemoteVerifier) (*grpc.ClientConn, error)
}

//go:generate mockery -dir . -name ConnectionMapper -case underscore -output ./mocks/

// ConnectionMapper maps certificates to connections
type ConnectionMapper interface {
	Lookup(cert []byte) (*grpc.ClientConn, bool)
	Put(cert []byte, conn *grpc.ClientConn)
	Remove(cert []byte)
}

// ConnectionStore stores connections to remote nodes
type ConnectionStore struct {
	certsByEndpoints atomic.Value
	lock             sync.RWMutex
	Connections      ConnectionMapper
	dialer           SecureDialer
}

// NewConnectionStore creates a new ConnectionStore with the given SecureDialer
func NewConnectionStore(dialer SecureDialer) *ConnectionStore {
	connMapping := &ConnectionStore{
		Connections: make(ConnByCertMap),
		dialer:      dialer,
	}
	return connMapping
}

// verifyHandshake returns a predicate that verifies that the remote node authenticates
// itself with the given TLS certificate
func (c *ConnectionStore) verifyHandshake(endpoint string, certificate []byte) RemoteVerifier {
	return func(rawCerts [][]byte, verifiedChains [][]*x509.Certificate) error {
		if bytes.Equal(certificate, rawCerts[0]) {
			return nil
		}
		return errors.Errorf("certificate presented by %s doesn't match any authorized certificate", endpoint)
	}
}

// Disconnect closes the gRPC connection that is mapped to the given certificate
func (c *ConnectionStore) Disconnect(expectedServerCert []byte) {
	c.lock.Lock()
	defer c.lock.Unlock()

	conn, connected := c.Connections.Lookup(expectedServerCert)
	if !connected {
		return
	}
	conn.Close()
	c.Connections.Remove(expectedServerCert)
}

// Connection obtains a connection to the given endpoint and expects the given server certificate
// to be presented by the remote node
func (c *ConnectionStore) Connection(endpoint string, expectedServerCert []byte) (*grpc.ClientConn, error) {
	c.lock.RLock()
	conn, alreadyConnected := c.Connections.Lookup(expectedServerCert)
	c.lock.RUnlock()

	if alreadyConnected {
		return conn, nil
	}

	// Else, we need to connect to the remote endpoint
	return c.connect(endpoint, expectedServerCert)
}

// connect connects to the given endpoint and expects the given TLS server certificate
// to be presented at the time of authentication
func (c *ConnectionStore) connect(endpoint string, expectedServerCert []byte) (*grpc.ClientConn, error) {
	c.lock.Lock()
	defer c.lock.Unlock()
	// Check again to see if some other goroutine has already connected while
	// we were waiting on the lock
	conn, alreadyConnected := c.Connections.Lookup(expectedServerCert)
	if alreadyConnected {
		return conn, nil
	}

	v := c.verifyHandshake(endpoint, expectedServerCert)
	conn, err := c.dialer.Dial(endpoint, v)
	if err != nil {
		return nil, err
	}

	c.Connections.Put(expectedServerCert, conn)
	return conn, nil
}

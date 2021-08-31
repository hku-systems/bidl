import yaml
import socket
import sys
import os

def LOG_ERROR(msg):
    pass
    # print('\33[40;31m\33[7mERROR: ' + msg)


with open('config-' + sys.argv[1] + '.yaml', 'r') as f:
    config = yaml.safe_load(f)


home = os.getenv('HOME') + '/'
peers = config['peers']['count']
template = config['peers']['template']
project = config['project']
hosts = config['hosts']
logging = config['logging']
port = '7051'
bootstrap = project + '_' + template + str(config['peers']['bootstrap']) + ':' + port
network = config['network']
tls = config['tls']
crypto = home + config['crypto']
orderer = config['orderer']
cli = config['cli']
tape = config['tape']
artifacts = home + config['artifacts']

services = []

fabric = {
    'version': '3',
    'networks': {
        network: {
            'external': True,
        }
    },
    'services': {

    }
}

if peers != len(hosts): 
    LOG_ERROR("config.yaml: peers != len(host)")
    # sys.exit(1)


# orderer
services.append(orderer['name'])
orderer_obj = {
    'container_name': orderer['name'], 
    'image': orderer['image'],
    'deploy': {
        'placement': {
            'constraints': [
                'node.hostname==' + orderer['host'], 
            ]
        }
    },
    'environment': [
        'GOPATH=/opt/gopath',
        'ORDERER_GENERAL_LOGLEVEL=' + logging, # is this right?
        'ORDERER_GENERAL_LEDGERTYPE=ram',
        'FABRIC_LOGGING_SPEC=' + logging, 
        'ORDERER_GENERAL_LISTENADDRESS=0.0.0.0',
        'ORDERER_GENERAL_GENESISMETHOD=file',        
        'ORDERER_GENERAL_GENESISFILE=/var/hyperledger/orderer/orderer.genesis.block', 
        'ORDERER_GENERAL_LOCALMSPID=OrdererMSP',
        'ORDERER_GENERAL_LOCALMSPDIR=/var/hyperledger/orderer/msp',
        'ORDERER_GENERAL_TLS_ENABLED=' + tls,
        'ORDERER_GENERAL_TLS_PRIVATEKEY=/var/hyperledger/orderer/tls/server.key',
        'ORDERER_GENERAL_TLS_CERTIFICATE=/var/hyperledger/orderer/tls/server.crt',
        'ORDERER_GENERAL_TLS_ROOTCAS=/var/hyperledger/orderer/tls/ca.crt', 
    ],
    'working_dir': '/opt/gopath/src/github.com/hyperledger/fabric',
    'command': 'bash -c "mkdir -p /var/hyperledger/production/orderer && orderer"', 
    'volumes': [
        artifacts + '/genesis.block:/var/hyperledger/orderer/orderer.genesis.block',
        crypto + '/ordererOrganizations/example.com/orderers/orderer.example.com/msp:/var/hyperledger/orderer/msp', 
        crypto + '/ordererOrganizations/example.com/orderers/orderer.example.com/tls:/var/hyperledger/orderer/tls',
    ], 
    'networks': [network, ], 
}
fabric['services']['orderer'] = orderer_obj


# peers
for i in range(peers):
    obj = {}
    peer = template + str(i)
    peername = project + '_' + peer
    services.append(peer)
    obj = {
        'container_name': peer,
        'image': config['peers']['image'],
        'deploy': {
            'placement': {
                'constraints': [
                    'node.hostname==' + hosts[i] ,
                ]
            }
        },
        'environment': [
            'GOPATH=/opt/gopath',
            'CORE_PEER_ID=' + peername,
            'CORE_LOGGING_LEVEL=' + logging,
            'CORE_PEER_ADDRESS=0.0.0.0:7051',
            'CORE_PEER_GOSSIP_BOOTSTRAP=' + bootstrap,
            'CORE_PEER_GOSSIP_EXTERNALENDPOINT=' + peername + ":" + port, 
            'CORE_PEER_LOCALMSPID=Org1MSP',  # TODO
            'CORE_VM_ENDPOINT=unix:///host/var/run/docker.sock',
            'CORE_VM_DOCKER_HOSTCONFIG_NETWORKMODE=' + network,
            'FABRIC_LOGGING_SPEC=' + logging,
            'CORE_PEER_TLS_ENABLED=' + tls,
            'CORE_PEER_GOSSIP_USELEADERELECTION=false',
            'CORE_PEER_GOSSIP_ORGLEADER=true', 
            'CORE_PEER_PROFILE_ENABLED=false',
            'CORE_PEER_TLS_CERT_FILE=/etc/hyperledger/fabric/tls/server.crt',
            'CORE_PEER_TLS_KEY_FILE=/etc/hyperledger/fabric/tls/server.key',
            'CORE_PEER_TLS_ROOTCERT_FILE=/etc/hyperledger/fabric/tls/ca.crt',
        ],
        'working_dir': '/opt/gopath/src/github.com/hyperledger/fabric/peer',
        'command': 'bash -c "mkdir -p /var/hyperledger/production && peer node start"', 
        'volumes': [
            '/var/run/:/host/var/run/',
            crypto + '/peerOrganizations/org1.example.com/peers/' + peer + '.org1.example.com/msp:/etc/hyperledger/fabric/msp', 
            crypto + '/peerOrganizations/org1.example.com/peers/' + peer + '.org1.example.com/tls:/etc/hyperledger/fabric/tls',
        ], 
        'networks': [network, ], 
    } 
    fabric['services'][peer] = obj


# cli
cli_obj = {
    'container_name': cli['name'], 
    'image': cli['image'],
    'deploy': {
        'placement': {
            'constraints': [
                'node.hostname==' + socket.gethostname(), 
            ]
        }
    },
    'tty': True,
    'stdin_open': True, 
    'environment': [
        'GOPATH=/opt/gopath',
        'FABRIC_LOGGING_SPEC=' + logging, 
        'CORE_PEER_ID=' + cli['name'], 
        'CORE_PEER_ADDRESS=0.0.0.0:' + port, 
        'CORE_PEER_LOCALMSPID=Org1MSP',
        'CORE_PEER_TLS_ENABLED=' + tls,
        'CORE_PEER_TLS_CERT_FILE=/opt/gopath/src/github.com/hyperledger/fabric/peer/organizations/peerOrganizations/org1.example.com/peers/peer0.org1.example.com/tls/server.crt', 
        'CORE_PEER_TLS_KEY_FILE=/opt/gopath/src/github.com/hyperledger/fabric/peer/organizations/peerOrganizations/org1.example.com/peers/peer0.org1.example.com/tls/server.key', 
        'CORE_PEER_TLS_ROOTCERT_FILE=/opt/gopath/src/github.com/hyperledger/fabric/peer/organizations/peerOrganizations/org1.example.com/peers/peer0.org1.example.com/tls/ca.crt', 
        'CORE_PEER_MSPCONFIGPATH=/opt/gopath/src/github.com/hyperledger/fabric/peer/organizations/peerOrganizations/org1.example.com/users/Admin@org1.example.com/msp', 
    ],
    'working_dir': '/opt/gopath/src/github.com/hyperledger/fabric/peer',
    'command': '/bin/bash', 
    'volumes': [
        '/var/run/:/host/var/run/',
        home + cli['chaincode'] + ':/opt/gopath/src/github.com/chaincode', 
        crypto + ':/opt/gopath/src/github.com/hyperledger/fabric/peer/organizations',
        home + cli['scripts'] + ':/opt/gopath/src/github.com/hyperledger/fabric/peer/scripts',
        artifacts + ':/opt/gopath/src/github.com/hyperledger/fabric/peer/channel-artifacts', 
    ], 
    'depends_on': services, 
    'networks': [network, ], 
}

fabric['services']['cli'] = cli_obj
# services.append(cli['name'])

tape_obj = {
    'container_name': tape['name'], 
    'image': tape['image'],
    'deploy': {
        'placement': {
            'constraints': [
                'node.hostname==' + socket.gethostname(), 
            ]
        }
    },
    'tty': True,
    'stdin_open': True, 
    'working_dir': '/',
    'command': '/bin/sh', 
    'volumes': [
        home + tape['config'] + ':/config.yaml',
        home + tape['organizations'] + ':/organizations',
    ], 
    # 'depends_on': services, 
    'networks': [network, ], 
}
fabric['services']['tape'] = tape_obj


with open('docker-compose-' + sys.argv[1] + '.yaml', 'w', encoding='utf8') as f:
    yaml.dump(fabric, f, default_flow_style=False, allow_unicode=True)


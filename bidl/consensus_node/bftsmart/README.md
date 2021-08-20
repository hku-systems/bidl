# BIDL README

## How to build

```shell
ant
```

## How to run

Run 4 nodes on localhost:

```shell
bash ./scripts/local_start_docker.sh 4
```

Run nodes on different servers (e.g., distributed).

1. First list server IP in `scripts/servers`.
2. Start 4 nodes on these servers.

```shell
bash ./scripts/deploy_smart_docker_new.sh 4
```

To test BIDL, we can let clients to directly multicast transactions to all nodes by (no sequencer):

```shell
ant client
```

This client generate transactions with random payloads of certain size (can be
modified in `bftsmart.demo.bidl.MulticastClient`). 


When using BIDL in real systems, the client sends transactions to the sequencer
via UDP unicast, and the sequencer multicast the transactions to all nodes. The
sequencer code can be found in `./sequencer`.

Therefore, letting the client to directly multicast transactions to all nodes
does not affect the throughput and latency (sequencer adds less than 1ms extra
latency, which can be ignored).

In ubuntu's default setting, the UDP buffer is too small and UDP packets will be
discarded when the transaction rate is high.

Please enlarge a server's UDP buffer before running BIDL, this must be run on
all servers, and reboot is not needed:

```shell
sysctl -w net.core.rmem_max=262144000
```

To modify BFT-Smart's configuration, please refer to `config.md`.

## List of code modifications

A frontend receives UDP transactions from the sequencer or client

- BidlFrontend.java: 我将 invokeOrdered 改为了 openloop

Check transaction hash (and request lost transactions) after receiving transactions.

- Acceptor.java/checkTxHash() 如果有问题的话，这个可以先注释掉。那么所有BIDL新增的逻辑都不会跑了，仅仅保留 consensus-on-hash

Logic for requests lost transactions.

- ServerConnection.java/ReceiverThread: handle BIDL message (gapReq and gapFill)
- BidlMessage.java: define bidl message
- MessageFactory.java: define bidl message
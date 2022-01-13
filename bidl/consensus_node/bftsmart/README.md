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

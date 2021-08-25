# Artifact for paper #403 BIDL: A High-throughput, Low-latency Permissioned Blockchain Framework for Datacenter Networks

## Artifact summary

BIDL is a high-throughput and low-latency permissioned blockchain framework designed for the datacenter networks (or multiple datacenters connected with dedicated network cables). BIDL can achieve high performance with Byzantine failures of malicious nodes/clients. This artifact contains the implementation of BIDL, together with scripts for reproducing the main results of this work.

## Artifact Check-list

- Code link: <https://github.com/hku-systems/bidl>
- OS Version: Ubuntu 16.04 or 18.04.
- Docker version: >= 19.03.11.
- Python version: >= 3.6.8.
- Metrics: throughput and latency.
- Expected runtime: each trial of each data point takes about 2 minutes.

## Deployment

### Clone the codebase

```shell
git clone git@github.com:hku-systems/bidl.git
```
	
### Create the docker images

Prerequisite:

1. You need to have multiple host machines with LAN networks.
2. The machines should be able to SSH each other without password.
3. Each host machine should have docker engine installed.
4. The user should be able to execute docker command without `sudo` privilege, which can be achieved by `sudo usermod -aG docker (username)`.
5. The machines should join the same docker swarm cluster. If you haven't done so, you can follow [this Docker official guide](https://docs.docker.com/engine/swarm/swarm-tutorial/).
6. The machines need to support the IP multicast.

Configurations:

1. Create the docker overlay network with

```shell
docker network create -d overlay --attachable HLF
```

2. Configure system UDP buffer to 250 Mbytes (avoiding packet losses at the system network buffer) by

```shell
sysctl -w net.core.rmem_max=262144000
sysctl -w net.core.rmem_default=262144000
```

3. Setup the loopback interface for multicast with

```shell
sudo ifconfig lo multicast
sudo route add -net 224.0.0.0 netmask 240.0.0.0 dev lo
```

4. To run Fabric and Fastfabric, you need to set the `host` parameter (IP address) in `config-fabric.yaml` and `config-fastfabric.yaml`. Make sure the length of host list is at least equal to the number of peers. In the following experiments, our scripts will generate docker compose files according to those configuration files. 

5. To run streamchain, you need to set the IP addresses in `streamchain/setup/config.sh`. 

6. Build the docker images for Fabric and Fastfabric with the following command.
 
 ```shell
# Before you start, you need to replace the IP list in config.sh according to your experiment environment. 
bash setup.sh
```

You can also use our cluster for all experiments. Please feel free to contact us for the ssh private key to access our cluster.

## Experiments

### Experiment 0: Test run

This experiment tests the experimental environment. The following test script benchmarks BIDL with 1e5 transactions.

As BIDL relies on the IP multicast to disseminate transactions, the experimental environment needs a low packet loss rate. 

If there are no errors when running the script, and the script print `Ok!`, then your experimental environment has a low packet loss rate and can run all experiments.

- Command to run:

```shell
cd bidl
bash ./scripts/test_run.sh
```

### Experiment 1: End-to-end performance

This experiment runs BIDL/FastFabric/Hyperledger Fabric/StreamChain with the default smallbank workload and reports the end-to-end throughput/latency of each system.

- Command to run:

```shell
bash run.sh performance
```

### Experiment 2: Performance with different ratio of contended transactions

This experiment runs BIDL and FastFabric (achieves the best performance in Experiment 1) with the default smallbank workload with different contention ratios of transactions.

- Command to run:

```shell
bash run.sh contention
```

### Experiment 3: Performance with different ratio of non-deterministic transactions

This experiment runs BIDL and FastFabric (achieves the best performance in Experiment 1) with different ratio of non-deterministic transactions.

- Command to run:

```shell
bash run.sh nd 
```

### Experiment 4: Performance with malicious participants

This experiment runs BIDL and Hyperledger Fabric with malicious participants.  Because FastFabric and StreamChain are designed for crash-fault-tolerant scenarios, we do not evaluate these two systems in this experiment.

- Command to run:

```shell
bash run.sh malicious
```

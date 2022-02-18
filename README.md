# Artifact for paper #403 BIDL: A High-throughput, Low-latency Permissioned Blockchain Framework for Datacenter Networks

## Artifact summary

BIDL is a high-throughput and low-latency permissioned blockchain framework designed for the datacenter networks (or multiple datacenters connected with dedicated network cables). BIDL can achieve high performance with Byzantine failures of malicious nodes/clients. This artifact contains the implementation of BIDL and scripts for reproducing the main results of this work.

## Artifact Check-list

- Code link: <https://github.com/hku-systems/bidl>
- OS Version: Ubuntu 16.04 or 18.04.
- Docker version: >= 19.03.11.
- Python version: >= 3.6.8.
- Metrics: throughput and latency.
- Expected runtime: each trial of each data point takes about 2 minutes.

## Experiments

If otherwise specified, all BIDL's experiments run four consensus nodes and 50 normal nodes.

### Prepare

1. Please login to our cluster following [this page](https://github.com/hku-systems/bidl/blob/main/servers.md).
2. Please login to server 2 (sosp21ae@202.45.128.161) and go to `/home/sosp21ae/bidl/` to run all the experiments, where we have set up all necessary environments.
3. Each experiment will generate a figure in the `./figure` directory. You can download all generated figures to your computer by running `python3 ./tunnel.sh [private_key]` **on your computer**, which start an ssh tunnel and copy all files in `./figure` to your computer using `scp`.
4. When the script is running, you may see `END` multiple times. The script is still running; please do not suspend the script.

Please be noted that different evaluators cannot run experiments at the same time. This is because the docker containers started by other evaluators will have the same IP address and container name as yours. You can check whether other evaluators are running the experiments by `docker ps` to see if any running containers are named `fabric`,`normal_node`, `smart`, or `sequencer`.

### Experiment 1: End-to-end performance

#### Experiment 1-1: Performance of BIDL and baseline systems (50 mins)

This experiment runs BIDL/FastFabric/Hyperledger Fabric/StreamChain with the Smallbank workload and reports each system's end-to-end throughput vs. latency.

**Command to run:**

```shell
bash run.sh performance
```

**Output:**

- A pdf file named `./figure/performance.pdf`, containing the throughput vs. latency of BIDL and baseline systems.
- You can find the log file for generating figures in `./logs/[system_name]/performance/`.

**Expected results:**

- BIDL achieves the highest throughput among all systems;
- StreamChain can achieve the lowest latency;
- BIDL's latency is better than FastFabric and Hyperledger Fabric.

#### Experiment 1-2: Scalability of BIDL (10 mins)

This experiment runs BIDL with the increasing number of organizations; each organization contains one consensus node and one normal node.

**Command to run:**

```shell
bash run.sh scalability
```

**Output:**

- A pdf file named `./figure/scalability.pdf`, containing the end-to-end latency of BIDL.
- You can find the log file for generating figures in `./logs/bidl/scalability/`.

**Expected results:**

- BIDL's end-to-end latency first decreases with the number of organizations (the execution workload is amortized among all normal nodes, so the execution latency decreases with the number of organizations);
- BIDL's end-to-end latency then increases when the number of organizations continues to increase (consensus latency increases with the number of organizations).

### Experiment 2: Performance with different ratios of contended transactions (20 mins)

This experiment runs BIDL and FastFabric (achieves the best performance in Experiment 1) with the default Smallbank workload with different contention ratios of transactions.

**Command to run:**

```shell
bash run.sh contention
```

**Output:**

- A pdf file named `./figure/contention.pdf` contains the throughput of BIDL and FastFabric with different ratios of contended transactions.
- You can find the log file for generating figures in `./logs/[system_name]/contention/`.

**Expected results:**

- BIDL's effective throughput shows no obvious decline.
- FastFabric's effective throughput decreases with the increasing ratio of contented transactions.

### Experiment 3: Performance with different ratios of non-deterministic transactions (20 mins)

This experiment runs BIDL and FastFabric (achieves the best performance in Experiment 1) with different ratio of non-deterministic transactions.

**Command to run:**

```shell
bash run.sh nd 
```

**Output:**

- A pdf file named `./figure/nondeterminism.pdf` contains the throughput of BIDL and baseline systems under different ratios of non-deterministic transactions.
- You can find the log file for generating figures in `./logs/[system_name]/non-determinism/`.

**Expected results:**

- The effective throughput of BIDL and FastFabric all drop with the increasing ratio of non-deterministic transactions.

### Experiment 4: Performance with malicious participants (10 mins)

This experiment runs BIDL with a malicious broadcaster. The malicious broadcaster broadcasts transactions with crafted sequence numbers. These crafted transactions will cause conflicts (with transactions from the sequencer) in nodes' sequence number spaces, causing the consensus throughput to drop. Consensus nodes shepherd the malicious broadcaster, and add the broadcaster to the denylist.

**Command to run:**

```shell
bash run.sh malicious
```

**Output:**

- A pdf file named `./figure/malicious.pdf`, containing the throughput of BIDL under a malicious broadcaster during five views.
- You can find the log file for generating figures in `./logs/bidl/malicious/`.

**Expected results:**

- BIDL's throughput drops in `view 0` due to the malicious broadcaster's misbehavior.
- BIDL's throughput recovers in `view 2`. This is because consensus nodes detect the broadcaster's misbehavior in $f+1=2$ views (with different leaders) and add the broadcaster to the denylist.

## Deployment (if you don't use our cluster)

### Clone the codebase

```shell
git clone git@github.com:hku-systems/bidl.git
```

### Create the docker images

**Prerequisite:**

1. You need to have multiple host machines with LAN networks.
2. The machines should be able to SSH each other without the password.
3. Each host machine should have the docker engine installed.
4. The user should be able to execute docker command without `sudo` privilege, which can be achieved by `sudo usermod -aG docker $USER`.
5. The machines should join the same docker swarm cluster. If you haven't done so, you can follow [this Docker official guide](https://docs.docker.com/engine/swarm/swarm-tutorial/).
6. The machines need to support the IP multicast.

**Configurations:**

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

4. To run BIDL, you need to list the servers you used in `./bidl/scripts/servers`. Make sure the first server address is the `IP` of your current server (i.e., the server you login and run experiments). In the following experiments, our scripts will put the leader consensus node on the first server and retrieve performance data from this node.
5. To run Hyperledger Fabric and FastFabric, you need to set the `host` parameter (IP address) in `config-fabric.yaml` and `config-fastfabric.yaml`. Make sure the length of host list is at least equal to the number of peers. In the following experiments, our scripts will generate docker compose files according to those configuration files.
6. To run StreamChain, you need to set the IP addresses in `streamchain/setup/config.sh`.
7. Build the docker images for Hyperledger Fabric and FastFabric with the following command.

```shell
# Before you start, you need to replace the IP list in config.sh according to your experiment environment. 
bash setup.sh
```

8. Build and deploy the docker images for BIDL with the following command.

```shell
# Please replace the IP list in ./bidl/script/servers according to your experiment environment.
bash ./bidl/scripts/deploy_bidl.sh $num_of_consensus_nodes
```

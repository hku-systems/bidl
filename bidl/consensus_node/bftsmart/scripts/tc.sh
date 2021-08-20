#!/bin/bash
echo "Set traffic control"
for host in `cat ./scripts/servers`; do
    # ssh hkucs@${host} 'sudo tc qdisc add dev p2p1 root tbf rate 10240mbit burst 50mbit latency 400ms'
    ssh jqi@${host} 'echo qiji | sudo -S tc qdisc add dev enp5s0 root netem delay 100ms 90ms 25%'
done

# sudo tc qdisc add dev enp1s0d1 root tbf rate 10240mbit burst 50mbit latency 400ms

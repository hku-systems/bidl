#!/bin/bash
echo "Set iptables"

echo rdma4paxos | sudo -S iptables -A INPUT -s 10.22.1.8 -j ACCEPT

for host in `cat ./scripts/servers`; do
    ssh jqi@${host} 'echo qiji | sudo -S iptables -A INPUT -m statistic --mode random --probability 0.08 -j DROP'
done

echo rdma4paxos | sudo -S iptables -L INPUT 
echo rdma4paxos | sudo -S iptables -L OUTPUT

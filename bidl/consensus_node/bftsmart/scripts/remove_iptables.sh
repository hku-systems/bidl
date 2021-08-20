#!/bin/bash
echo "Remove iptables..."
for host in `cat ./scripts/servers`; do
    ssh jqi@${host} 'echo qiji | sudo -S iptables -F INPUT'
    ssh jqi@${host} 'echo qiji | sudo -S iptables -F OUTPUT'
done
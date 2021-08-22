#!/bin/bash
echo "Remove traffic control..."
for host in `cat ./scripts/servers`; do
    ssh jqi@${host} 'echo qiji | sudo -S tc qdisc del dev lo root'
done
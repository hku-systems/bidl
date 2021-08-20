#!/bin/bash
echo "Deploy to all servers..."
for host in `cat ./scripts/servers`; do
    rsync -ravzh --delete ./  hkucs@${host}:~/BIDL/bft-smart
done

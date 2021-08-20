# !/bin/bash
for host in `cat ./scripts/servers`; do
    ssh hkucs@${host} 'pkill -f bft'
done
pkill -f bft
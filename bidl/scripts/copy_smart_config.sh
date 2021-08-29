#!/bin/bash

echo "Copy configure files to all servers..."
script_dir=$(cd "$(dirname "$0")";pwd)
source $script_dir/env.sh
for host in `cat $base_dir/scripts/servers`; do
    echo $host
    ssh $user@${host} 'cd ~; rm -rf configs; mkdir configs;'
    scp -r $script_dir/configs/* $user@${host}:~/configs/
done
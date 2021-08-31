#!/bin/bash
set -eu
echo "setup BIDL environments..."
base_dir=./bidl
for i in 1 2 3 4 5 6 9 19; do
    host=10.22.1.$i
    ssh -t sosp21ae@${host} 'sudo apt install python3-pip'
done

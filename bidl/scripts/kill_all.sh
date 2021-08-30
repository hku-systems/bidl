# !/bin/bash
echo "Killing consensus nodes, normal nodes, and clients..."

script_dir=$(cd "$(dirname "$0")";pwd)
source $script_dir/env.sh

for host in `cat $base_dir/scripts/servers`; do
    echo $host
    ssh jqi@${host} 'docker stop $(docker ps -aq --filter name="smart"); docker rm $(docker ps -aq --filter name="smart");
    docker stop $(docker ps -aq --filter name="normal_node"); docker rm $(docker ps -aq --filter name="normal_node");
    docker stop $(docker ps -aq --filter name="bidl_client"); docker rm $(docker ps -aq --filter name="bidl_client")'
done

echo "Killing sequencer..."
pkill -f sequencer
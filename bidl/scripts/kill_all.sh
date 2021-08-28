# !/bin/bash
echo "Killing consensus nodes, normal nodes, and clients..."

for host in `cat ./scripts/servers`; do
    echo $host
    ssh jqi@${host} 'docker stop $(docker ps -aq --filter name="smart"); docker rm $(docker ps -aq --filter name="smart");
    docker stop $(docker ps -aq --filter name="normal_node"); docker rm $(docker ps -aq --filter name="normal_node");
    docker stop $(docker ps -aq --filter name="bidl_client"); docker rm $(docker ps -aq --filter name="bidl_client")'
done

echo "Killing sequencer..."
pkill -f sequencer
# !/bin/bash
echo "Killing consensus nodes..."
for host in `cat ./scripts/servers`; do
    echo $host
    ssh jqi@${host} 'docker stop $(docker ps -aq --filter name="smart"); docker rm $(docker ps -aq --filter name="smart")'
done

echo "Killing normal nodes..."
for host in `cat ./scripts/servers`; do
    echo $host
    ssh jqi@${host} 'docker stop $(docker ps -aq --filter name="normal_node"); docker rm $(docker ps -aq --filter name="normal_node")'
done

echo "Killing sequencer..."
pkill -f sequencer
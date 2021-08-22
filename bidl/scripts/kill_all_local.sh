# !/bin/bash
echo "Killing consensus nodes..."
docker stop $(docker ps -aq --filter name="smart"); docker rm $(docker ps -aq --filter name="smart")

echo "Killing normal nodes..."
docker stop $(docker ps -aq --filter name="normal_node"); docker rm $(docker ps -aq --filter name="normal_node")

echo "Killing sequencer..."
pkill -f sequencer
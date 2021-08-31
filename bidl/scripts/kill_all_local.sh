# !/bin/bash

echo "Killing consensus nodes..."
docker stop $(docker ps -aq --filter name="smart"); docker rm $(docker ps -aq --filter name="smart")

echo "Killing normal nodes..."
docker stop $(docker ps -aq --filter name="normal_node"); docker rm $(docker ps -aq --filter name="normal_node")

echo "Killing BIDL clients..."
docker stop $(docker ps -aq --filter name="bidl_client"); docker rm $(docker ps -aq --filter name="bidl_client")

echo "Killing sequencer..."
docker stop $(docker ps -aq --filter name="sequencer"); docker rm $(docker ps -aq --filter name="sequencer")
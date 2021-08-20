#!/bin/bash
echo "Deploy docker image to all servers..."
for host in `cat ./scripts/servers`; do
    echo $host
    scp smart.tar jqi@${host}:~/
    ssh jqi@${host} 'docker rm $(docker ps -aq --filter name="smart"); docker rmi smart; docker load --input smart.tar'
done
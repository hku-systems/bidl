# !/bin/bash
echo "Stoping running containers"
for host in `cat ./scripts/servers`; do
    echo $host
    ssh jqi@${host} 'docker stop $(docker ps -aq --filter name="smart"); docker rm $(docker ps -aq --filter name="smart")'
done
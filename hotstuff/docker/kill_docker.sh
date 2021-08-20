# !/bin/bash
echo "Stoping running containers"

user=$(cat docker/config.sh | awk -F '#' '{print $1}' | grep "user"  | awk -F '=' '{print $2}' | awk '$1=$1')

hosts=($(cat ./docker/servers))
len=${#hosts[*]}
it=0
for host in ${hosts[*]}; do
    echo $host
    let it=$it+1
    if [ $it -eq $len ]; then  
        ssh ${user}@${host} 'docker stop $(docker ps -aq --filter name="hotstuff") > /dev/null 2>&1 ; docker rm $(docker ps -aq --filter name="hotstuff") > /dev/null 2>&1'
    else
        ssh -f -n ${user}@${host} 'docker stop $(docker ps -aq --filter name="hotstuff") > /dev/null 2>&1 ; docker rm $(docker ps -aq --filter name="hotstuff") > /dev/null 2>&1'
    fi
done
docker stop c0
sleep 2
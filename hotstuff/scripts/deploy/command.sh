#!/bin/bash

user=$(cat config.sh | awk -F '#' '{print $1}' | grep "user"  | awk -F '=' '{print $2}' | awk '$1=$1')

hosts=($(cat servers))
len=${#hosts[*]}
for host in ${hosts[*]}; do
    echo $host
    # a=$(ssh ${user}@${host}  'lsb_release -a' # 'lscpu | grep On-line' | awk -F '-' '{print $3}')
    ssh ${user}@${host}  $@ # 'lscpu | grep On-line' | awk -F '-' '{print $3}')
    # echo $(($a+1))
done
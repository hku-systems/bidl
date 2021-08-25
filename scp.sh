#!/bin/bash 


. config.sh
for host in ${hosts[@]}; do
    echo "###  $host  ###"
    scp $1 $USER@$host:~/
done

echo done


#!/bin/bash

. config.sh

for host in ${hosts[@]}; do
    echo "###  $host  ###"
    ssh -tt $USER@$host "$1" 
done

echo done


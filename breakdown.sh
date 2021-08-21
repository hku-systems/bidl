#!/bin/bash

log=$1
if [ $# -eq 1 ]; then 
    grep  "Exit chaincode: name" $log | grep "smallbank" | awk '{print $15}' | awk -F m '{print $1}' | cut -c 2- > endorse.log
    grep  "Committed" $log | awk '{print $19}' | awk -F m '{print $1}' > committed.log
    echo -n "endorse "
    cat endorse.log | python3 breakdown.py
    echo -n "commit "
    cat committed.log | python3 breakdown.py
elif [ $# -eq 2 ]; then 
    if [ $2 == "endorse" ]; then 
        grep  "Exit chaincode: name" $log | grep "smallbank" | awk '{print $15}' | awk -F m '{print $1}' | cut -c 2- > endorse.log
        echo -n "endorse "
        cat endorse.log | python3 breakdown.py endorse
    elif [ $2 == "commit" ]; then 
        grep  "Committed" $log | awk '{print $19}' | awk -F m '{print $1}' > committed.log
        echo -n "commit "
        cat committed.log | python3 breakdown.py commit

    fi

fi
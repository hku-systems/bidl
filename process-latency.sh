#!/bin/bash
cur=$1
round=$2

# for round in `seq 1 8`; do 
    for peer in `seq 0 4`; do 
        echo peer$peer
        bash breakdown.sh $cur/round_${round}_peer${peer}.log $3
    done
    # done

echo -n "orderer "
cat $cur/round_${round}_orderer.log | python3 orderer.py
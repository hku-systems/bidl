#!/bin/bash
log=$1

grep  "Exit chaincode: name" $log | grep "smallbank" | awk '{print $15}' | awk -F m '{print $1}' | cut -c 2- > endorse.log
grep  "Committed" $log | awk '{print $19}' | awk -F m '{print $1}' > committed.log

echo "endorse: "
cat endorse.log | python3 breakdown.py
echo "committed: "
cat committed.log | python3 breakdown.py


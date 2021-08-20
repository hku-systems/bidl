# !/bin/bash
rm -f stats.csv
while true
do
    docker stats $(docker ps -aq --filter name="smart") --no-stream >> stats.csv
    sleep 1
done
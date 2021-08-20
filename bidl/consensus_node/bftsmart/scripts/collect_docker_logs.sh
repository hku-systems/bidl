#!/bin/bash

rm -rf logs

mkdir logs

docker logs smart0 > logs/logs_0.log 2>&1 &
docker logs smart1 > logs/logs_1.log 2>&1 &
docker logs smart2 > logs/logs_2.log 2>&1 &
docker logs smart3 > logs/logs_3.log 2>&1 &

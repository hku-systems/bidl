#!/bin/bash

echo "Stop peers"
docker-compose -f docker-compose.yaml down

echo "Rebuilding image"
ant
cp ./scripts/configs/system_4.config ./config/system.config
rm -f config/currentView
rm -f smart.tar
rm -rf logs
docker image build -t smart .

echo "Start 4 local peers"
docker-compose -f docker-compose.yaml up -d
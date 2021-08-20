#!/bin/bash

echo "*********************************************************"
echo "Building image"
echo "*********************************************************"

ant
cp ./scripts/configs/system_4.config ./config/system.config
rm -f config/currentView
rm -f smart.tar
rm -rf logs
docker image build -t smart .
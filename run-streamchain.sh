#!/bin/bash
export GOPATH=$HOME/go
rm -rf logs/streamchain
mkdir -p logs/streamchain
cd streamchain/setup
bash teardown.sh 
bash reconfigure.sh
bash run_main.sh
# bash run_batchwrite.sh

mv logs/processed/* ../../logs/streamchain
rm -rf ../../logs/streamchain/txval.log
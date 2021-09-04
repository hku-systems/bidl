#!/bin/bash
mkdir -p logs/streamchain
cd streamchain/setup
bash teardown.sh 
bash reconfigure.sh
bash run_main.sh
# bash run_batchwrite.sh

mv logs/processed/* ../../logs/streamchain
rm -rf ../../logs/streamchain/txval.log
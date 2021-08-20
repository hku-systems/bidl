#!/bin/bash
mkdir -p logs/streamchain
cd streamchain/setup
bash run_main.sh

mv logs/processed ../../logs/streamchain/
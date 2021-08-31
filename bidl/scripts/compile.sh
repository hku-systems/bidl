#!/bin/bash

script_dir=$(cd "$(dirname "$0")";pwd)
source $script_dir/env.sh

cd $sequencer_dir
echo "Compiling sequencer..."
g++ -std=c++11 sequencer_multicast.cpp -o sequencer

cd $smart_dir
echo "Compiling consensus node..."
ant clean
ant

cd $normal_node_dir
echo "Compiling normal node..."
go build -o server ./cmd/server
go build -o client ./cmd/client

cd $base_dir
#!/bin/bash
set -eu

script_dir=$(cd "$(dirname "$0")";pwd)
source $script_dir/env.sh

rm -f $smart_dir/config/currentView

rm -rf $smart_dir/logs
rm -rf $base_dir/logs

rm -f $smart_dir/smart.tar

rm -f $base_dir/smart.tar
rm -f $base_dir/sequencer.tar
rm -f $base_dir/normal_node.tar

echo "Building consensus node image..."
cd $smart_dir
docker image build --no-cache -t smart:latest .
docker save -o $base_dir/smart.tar smart:latest

echo "Building sequencer image..."
cd $sequencer_dir
docker image build --no-cache -t sequencer:latest .
docker save -o $base_dir/sequencer.tar sequencer:latest

echo "Building normal node image..."
cd $normal_node_dir
docker image build --no-cache -t normal_node:latest .
docker save -o $base_dir/normal_node.tar normal_node:latest

cd $base_dir
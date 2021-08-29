#!/bin/bash
script_dir=$(cd "$(dirname "$0")";pwd)
source $script_dir/env.sh

echo "Building consensus node image"
rm -f $smart_dir/config/currentView

rm -rf $smart_dir/logs
rm -rf $base_dir/logs

rm -f $smart_dir/smart.tar
rm -f $base_dir/smart.tar

cd $smart_dir
docker image build -t smart .
docker save -o $base_dir/smart.tar smart:latest

echo "Building normal node image"
cd $normal_node_dir
docker image build -t normal_node .
docker save -o $base_dir/normal_node.tar normal_node
cd $base_dir
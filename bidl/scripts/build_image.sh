#!/bin/bash

echo "Building consensus node image"
rm -f $smart_dir/config/currentView

rm -rf $smart_dir/logs
rm -rf $dir/logs

rm -f $smart_dir/smart.tar
rm -f $dir/smart.tar

cd $smart_dir
# docker image build -t smart $smart_dir
docker image build -t smart .
docker save -o $dir/smart.tar smart

echo "Building normal node image"
cd $normal_node_dir
docker image build -t normal_node .
docker save -o $dir/normal_node.tar normal_node
cd $dir
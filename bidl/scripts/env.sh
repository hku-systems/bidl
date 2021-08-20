#!/bin/bash -e
export GO111MODULE="on"
script_dir=$(cd "$(dirname "$0")";pwd)
base_dir=$(cd $script_dir/..; pwd)
smart_dir="$base_dir/consensus_node/bftsmart"
sequencer_dir="$base_dir/sequencer"
normal_node_dir="$base_dir/normal_node"
log_dir="$base_dir/logs"
echo "artifact base dir: "$base_dir
echo "script dir: "$script_dir
echo "normal node dir: "$normal_node_dir
echo "consensus node dir: "$normal_node_dir
echo "sequencer dir: "$sequencer_dir
echo "log dir: " $log_dir
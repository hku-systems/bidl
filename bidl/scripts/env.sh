#!/bin/bash -e
script_dir=$(cd "$(dirname "$0")";pwd)
base_dir=$(cd $script_dir/..; pwd)

# file=${BASH_SOURCE[0]}
# source=$(dirname $file)
# script_dir=$(pwd)${source: 1}
# base_dir=$(dirname $script_dir)
smart_dir="$base_dir/consensus_node/bftsmart"
sequencer_dir="$base_dir/sequencer"
normal_node_dir="$base_dir/normal_node"
log_dir="$base_dir/logs"
export GO111MODULE="on"

echo "artifact base dir: "$base_dir
echo "script dir: "$script_dir
echo "normal node dir: "$normal_node_dir
echo "consensus node dir: "$smart_dir
echo "sequencer dir: "$sequencer_dir
echo "log dir: " $log_dir
#!/bin/bash -e

script_dir=$(cd "$(dirname "$0")";pwd)
bash $script_dir/env.sh

echo "--------------------------------"
echo "artifact base dir: "$base_dir
echo "script dir: "$script_dir
echo "normal node dir: "$normal_node_dir
echo "consensus node dir: "$normal_node_dir
echo "sequencer dir: "$sequencer_dir
echo "log dir: " $log_dir
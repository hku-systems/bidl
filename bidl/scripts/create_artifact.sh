#!/bin/bash
set -e

script_dir=$(cd "$(dirname "$0")";pwd)
source $script_dir/env.sh

bash $base_dir/scripts/build_image.sh

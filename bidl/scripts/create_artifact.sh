#!/bin/bash
set -e

script_dir=$(cd "$(dirname "$0")";pwd)
source $script_dir/env.sh

source $base_dir/scripts/compile.sh
source $base_dir/scripts/build_image.sh
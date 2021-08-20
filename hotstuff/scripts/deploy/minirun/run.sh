#!/bin/bash

basedir="$(dirname $(realpath "${BASH_SOURCE[0]}"))"
node_group="${node_group:-nodes}"
node_setup_group="${node_setup_group:-nodes_setup}"
node_file="${node_file:-$basedir/nodes.ini}"
group_vars="${group_vars:-$basedir/group_vars}"
app_module="${app_module:-$basedir/app/}"
ansible="$basedir/ansible"

build_tasks="${build_tasks:-$app_module/build.yml}"
reset_tasks="${reset_tasks:-$app_module/reset.yml}"
run_tasks="${run_tasks:-$app_module/run.yml}"
setup_tasks="${setup_tasks:-$app_module/_setup.yml}"
forced=false
export ANSIBLE_CONFIG="$basedir/ansible.cfg"

function die { echo "error: $1"; exit 1; }
function print_help {
echo "Usage: $0 [--group] [--group_vars] [--nodes] [--help] [--force] COMMAND RUNID

    --help                      show this help and exit
    --group                     specify the ansible group name
    --group_vars                specify the group_vars dir
    --nodes                     specify the ansible inventory (.ini or .yml/.yaml)
    --force                     force the operation

COMMAND
    setup                       setup the environment before launch (once is enough)
    new                         launch the nodes and create a workdir with RUNID
    check                       check whether nodes are still running
    stop                        stop all nodes
    fetch                       fetch back the generated files from the remote
    start                       start all nodes (if they're not running)
    reset                       force stop all nodes and reset their states
"
    exit 0
}

function check_argnum {
    argnum=$(($# - 1))
    [[ "$1" -eq "$argnum" ]] || die "incorrect argnum: got $argnum, $1 expected"
}

function _check_id {
    id=${1%/}
    [[ "$id" =~ ^[0-9a-zA-Z_-|]+$ ]]
}

function check_id {
    id=${1%/}
    (_check_id "$id" && [[ -f "$id/nodes" ]]) || die "invalid runid: \"$id\""
    workdir="$id"
}

function try_lock {
    set -o noclobber && echo > "$1/lock" || die "another run.sh is busy"
}

function unlock {
    rm "$1/lock"
}

function ansible {
    workdir="$1"
    action="$2"
    ansible-playbook -i "$workdir/nodes" \
        --extra-vars \
            "run_id=$workdir \
            node_group=$node_group \
            build_tasks=$build_tasks \
            reset_tasks=$reset_tasks \
            run_tasks=$run_tasks \
            forced=$forced" \
        -M "$app_module" \
        "$ansible/$action.yml"
}

function _setup {
    ansible-playbook -i "$node_file" \
        --extra-vars \
            "node_setup_group=$node_setup_group\
            build_tasks=$build_tasks \
            setup_tasks=$setup_tasks" \
        "$ansible/_setup.yml" "$@"
}

function _new {
    _check_id "$1"
    workdir="$id"
    if [[ -a "$workdir" ]]; then
        die "$workdir already exists"
    fi
    mkdir -p "$workdir"
    cp "$node_file" "$workdir/nodes"
    cp -r "$group_vars" "$workdir"
    try_lock "$workdir"
    ansible "$workdir" start
    unlock "$workdir"
}

function _start {
    check_id "$1"
    try_lock "$workdir"
    ansible "$workdir" start
    unlock "$workdir"
}

function _stop {
    check_id "$1"
    try_lock "$workdir"
    ansible "$workdir" stop
    unlock "$workdir"
}

function _check {
    check_id "$1"
    try_lock "$workdir"
    ansible "$workdir" check
    unlock "$workdir"
}

function _fetch {
    check_id "$1"
    try_lock "$workdir"
    ansible "$workdir" fetch
    unlock "$workdir"
}

function _reset {
    check_id "$1"
    try_lock "$workdir"
    ansible "$workdir" reset
    unlock "$workdir"
}

getopt --test > /dev/null
[[ $? -ne 4 ]] && die "getopt unsupported"

SHORT=
LONG='\
group:,\
group-vars:,\
nodes:,\
force,\
help'

PARSED=$(getopt --options "$SHORT" --longoptions "$LONG" --name "$0" -- "$@")
[[ $? -ne 0 ]] && exit 1
eval set -- "$PARSED"

while true; do
    case "$1" in
        --group) node_group="$2"; shift 2;;
        --group-vars) group_vars="$2"; shift 2;;
        --help) print_help; shift 1;;
        --nodes) node_file="$2"; shift 2;;
        --force) forced=true; shift 1;;
        --) shift; break;;
        *) die "internal error";;
    esac
done
cmd="$1"
shift 1
case "$cmd" in
    setup) check_argnum 0 "$@" && _setup ;;
    new) check_argnum 1 "$@" && _new "$1" ;;
    start) check_argnum 1 "$@" && _start "$1" ;;
    stop) check_argnum 1 "$@" && _stop "$1" ;;
#    status) check_argnum 1 "$@" && status_all "$1" ;;
    check) check_argnum 1 "$@" && _check "$1" ;;
    fetch) check_argnum 1 "$@" && _fetch "$1" ;;
#    wait) check_argnum 1 "$@" && wait_all "$1" ;;
    reset) check_argnum 1 "$@" && _reset "$1" ;;
#    exec) check_argnum 2 "$@" && exec_all "$1" "$2" ;;
    *) print_help ;;
esac

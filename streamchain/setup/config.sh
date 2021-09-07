# Setup-related parameters
export event="10.22.1.1"
export add_events=""
export peers="10.22.1.1 10.22.1.3 10.22.1.4 10.22.1.5"
export orderers="10.22.1.6"
export user=$USER
export goexec="/usr/local/go/bin"
export bm_path="$HOME/benchmark"
export event_dir="$user@$event:$bm_path"
export run_event="ssh $user@$event cd $bm_path;"
export txs=1
export mode="streamchain_ff"
export orderer_data="/$HOME/benchmark/ramdisk/$user"
export peer_data="/$HOME/benchmark/tmp/$user"
export statedb_data="/$HOME/benchmark/ramdisk/$user"
export raft="true"

# Streamchain related variables
export STREAMCHAIN_ADDBLOCK=''
export STREAMCHAIN_PIPELINE='16'
export STREAMCHAIN_VSCC=''
export STREAMCHAIN_SYNCDB='true'
export STREAMCHAIN_WRITEBATCH='500'

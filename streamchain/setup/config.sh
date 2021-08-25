# Setup-related parameters
export event="ms0837.utah.cloudlab.us"
export add_events=""
export peers="ms0835.utah.cloudlab.us ms0838.utah.cloudlab.us ms0812.utah.cloudlab.us ms0808.utah.cloudlab.us"
export orderers="ms0837.utah.cloudlab.us"
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

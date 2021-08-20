# Usage: setup_network.sh
export FABRIC_LOGGING_SPEC=CRITICAL

#. config.sh

echo 'Generating core.yaml for every peer..'
leader=true
i=0
for p in $peers; do 
    if [ "$leader" = true ] ; then
        ./gen_scripts/gen_core.sh $p 'true' $i
        leader=false
    else
        ./gen_scripts/gen_core.sh $p 'false' $i
    fi
    i=$(($i+1))
done

echo 'Generating orderer.yaml for orderers..'
i=0
for o in $orderers ; do
	./gen_scripts/gen_orderer.sh $o $i
	i=$(($i+1))
done

echo 'Generate crypto-config.yaml..'
./gen_scripts/gen_crypto_config.sh

echo 'Generate crypto files..'
./generate_crypto.sh

echo 'Generating configtx.yaml..'
./gen_scripts/gen_configtx.sh

echo 'Generating config transaction files..'
./generate_configtx.sh


mode=$1
clients=$2
events=$3
size=$4
extra1=$5
extra2=$6

echo 'Pulling logs from all nodes..'
for o in $orderers ; do
	mkdir -p logs/$o/
	scp -q $user@$o:$bm_path/orderer.log logs/$o/
done

for p in $peers ; do
	mkdir -p logs/$p/
	scp -q $user@$p:$bm_path/peer.log logs/$p/
done

mkdir -p logs/processed

echo 'Applying transformations..'
./proc_validation.sh $mode $clients $events $size $extra1 $extra2
./proc_endorsement.sh $mode $clients $events $size $extra1 $extra2
./proc_ordering.sh $mode $clients $events $size $extra1 $extra2
./proc_txval.sh $mode $clients $events $size $extra1 $extra2

unique=$(echo $peers $orderers | sort -nu)

echo 'Archive logs..'
mkdir -p logs/"$1_$2_$3_$4_$5_$6"
cd logs ; cp -r $unique "$1_$2_$3_$4_$5_$6/" ; rm -rf $peers $orderers

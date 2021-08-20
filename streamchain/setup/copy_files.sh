# Usage: copy_files.sh

. config.sh

echo 'Copying executables..'

for i in $peers $orderers ; do
    ssh $user@$i "mkdir -p $bm_path"
done

echo "Copying $mode binaries.."

for peer in $peers ; do
	scp -rq bin/$mode/peer $user@$peer:$bm_path
done

for orderer in $orderers ; do
	scp -rq bin/$mode/orderer $user@$orderer:$bm_path
done

for i in $peers $orderers ; do
	scp -q log.json $user@$i:$bm_path
done

echo 'Copying chaincode..'

chaincode="chaincode.go"

echo 'Deleting old Docker images'

for i in $event $add_events ; do
		docker rmi -f $(docker images | grep ^dev.* | awk '{print $1}')
done

if [ "$1" != "" ] ; then	
    chaincode=$1
fi

for i in $peers ; do
	ssh $user@$i "mkdir -p $GOPATH/src/github.com/chaincode/YCSB"
	scp -q ../code/ycsb/chaincode/$chaincode $user@$i:$GOPATH/src/github.com/chaincode/YCSB/
done

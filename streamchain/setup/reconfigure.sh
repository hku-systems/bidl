# Call this script BEFORE making changes in the role assignments in config.sh

. config.sh

if [ -z "$bm_path" ]
then
	echo "Variable \$bm_path is not set!"
else
	for i in $peers $orderers $event $add_event ; do
		ssh $user@$i "rm -rf $bm_path/*"
	done
fi



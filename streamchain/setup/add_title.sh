title=$1

line="<--------------- $title --------------->"

for file in logs/processed/*.log ; do
	echo $line >> $file
done

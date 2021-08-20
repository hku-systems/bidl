# Usage: gen_workload.sh #1:TEMPLATE_FILE #2:OUTPUT_FILE

. config.sh

template=$1
file=$2

len_p=$(wc -w <<< $peers)
len_o=$(wc -w <<< $orderers)

apeers=($peers)
aorderers=($orderers)

rm -f $file

temp='temp'

lines=$(grep -ch "^" $template)
let lines=lines-1
for i in `seq 0 $lines` ; do
	echo "${aorderers[$(($i % $len_o))]}:7050 ${apeers[$(($i % $len_p))]}:7051 crypto-config/ordererOrganizations/example.com/orderers/orderer$(($i % $len_o)).example.com/tls/ca.crt" >> $temp
done

paste -d " " $temp $template > $file

rm -f $temp

for e in $event $add_events ; do
	scp -q $file $user@$e:$bm_path
done

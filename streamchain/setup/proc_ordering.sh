for o in $orderers ; do
	cat logs/$o/orderer.log >> temp.txt
done

size=$(($3*$4))
blocks=$(($size/$txs))

grep ^ord0.* temp.txt | sort -k 2 -n -t ',' | tail -n $size > temp0.txt
grep ^ord1.* temp.txt | sort -u -k 3 -n -t ',' | tail -n $blocks | sed "h;:a;s/[^\n]\+/&/$txs;t;G;ba"> temp1.txt

cut=$(($size/10))
left=$(($size-$cut))

sed -n "$(($cut+1)),$left p" temp0.txt > temp01.txt
sed -n "$(($cut+1)),$left p" temp1.txt > temp11.txt

# TPUT calculation
val1=$(head -1 temp11.txt)
val2=$(tail -1 temp11.txt)
tput=$(echo "$val1,$val2,$(($left-$cut))" | awk -F',' '{ printf "%f", ($7 / ($5 - $2)) * 1000 * 1000 * 1000; }')

# Latency calulcation
avg=$(paste -d ',' temp01.txt temp11.txt | awk -F',' '{ sum += ($5-$2); n++ } END { printf "%f", (sum / n) / 1000000 }')

rm -f temp.txt temp0.txt temp1.txt temp01.txt temp11.txt

echo "$1,$2,$3,$4,$5,$6,$raft,$avg,$tput" >> logs/processed/ordering.log
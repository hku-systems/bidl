file="logs/$event/peer.log"

size=$((($3*$4)/$txs))

grep ^val0.* $file | sort -k3 -n -t ',' | tail -n $size > temp1.txt
grep ^val1.* $file | sort -k3 -n -t ',' | tail -n $size > temp2.txt
grep ^val2.* $file | sort -k3 -n -t ',' | tail -n $size > temp3.txt
grep ^val3.* $file | sort -k3 -n -t ',' | tail -n $size > temp4.txt

cut=$(($size/10))
left=$(($size-$cut))

sed -n "$(($cut+1)),$left p" temp1.txt > temp11.txt
sed -n "$(($cut+1)),$left p" temp2.txt > temp21.txt
sed -n "$(($cut+1)),$left p" temp3.txt > temp31.txt
sed -n "$(($cut+1)),$left p" temp4.txt > temp41.txt

# TPUT calculation
val1=$(head -1 temp41.txt)
val2=$(tail -1 temp41.txt)

tput=$(echo "$val1,$val2,$(($left-$cut)),$txs" | awk -F',' '{ printf "%f", ($7 / ($5 - $2)) * $8 * 1000 * 1000 * 1000; }')

# Latency calculation
first=$(paste -d ',' temp11.txt temp21.txt | awk -F',' '{ sum += ($5-$2); n++ } END { printf "%f", (sum / n) / 1000000; }' )
second=$(paste -d ',' temp21.txt temp31.txt | awk -F',' '{ sum += ($5-$2); n++ } END { printf "%f", (sum / n) / 1000000; }')
third=$(paste -d ',' temp31.txt temp41.txt | awk -F',' '{ sum += ($5-$2); n++ } END { printf "%f", (sum / n) / 1000000; }')

avg=$(paste -d ',' temp11.txt temp41.txt | awk -F',' '{ sum += ($5-$2); n++ } END { printf "%f", (sum / n) / 1000000; }')

rm -f temp1.txt temp2.txt temp3.txt temp4.txt temp11.txt temp21.txt temp31.txt temp41.txt

echo "$1,$2,$3,$4,$5,$6,$first,$second,$third,$avg,$tput" >> logs/processed/validation.log

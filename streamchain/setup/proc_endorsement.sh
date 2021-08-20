file="logs/$event/peer.log"

for p in $peers ; do
	cat logs/$p/peer.log | grep ^end.* >> temp.txt
done

sort -k2 -n -t ',' temp.txt | tail -n $4 > temp1.txt

cut=$(($4 / 10))
left=$(($4-$cut))

sed -n "$(($cut+1)),$left p" temp1.txt > temp2.txt

# TPUT calculation

val1=$(head -1 temp2.txt)
val2=$(tail -1 temp2.txt)

tput=$(echo "$val1,$val2,$(($left-$cut))" | awk -F',' '{ printf "%f", ($7 / ($5 - $2)) * 1000 * 1000 * 1000; }')

# Latency calculation

avg=$(awk -F',' '{ sum += $3; n++ } END { printf "%f", (sum / n) / 1000000 }' temp2.txt)

rm temp.txt temp1.txt temp2.txt

echo "$1,$2,$3,$4,$5,$6,$avg,$tput" >> logs/processed/endorsement.log

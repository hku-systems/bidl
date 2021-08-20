file="logs/$event/peer.log"

grep ^txval.* $file > temp.txt
info=$(awk -F',' '{ printf "%d,%f", $3, $4; }' temp.txt)

rm -f temp.txt

echo "$1,$2,$3,$4,$5,$6,$info" >> logs/processed/txval.log

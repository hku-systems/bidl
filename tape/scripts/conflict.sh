cat log.transactions | egrep "VALID|MVCC" | awk -F _ '{print $1,$4}' | sort -n -k 3 > __temp__
all=$(cat __temp__ | wc -l)
echo CONFLICT: $(cat __temp__ | grep "CONFLICT" | wc -l)/$all
echo VALID: $(cat __temp__ | grep -v "CONFLICT" | wc -l)/$all
rm __temp__

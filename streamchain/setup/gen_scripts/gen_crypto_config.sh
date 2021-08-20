# Usage: gen_crypto_config.sh

file=crypto-config.yaml
rm -f $file

echo "OrdererOrgs:

  - Name: Orderers
    Domain: example.com
    Specs:" >> $file

i=0
for o in $orderers ; do
echo "      - Hostname: orderer$i
        SANS:
        - $o" >> $file
i=$((i+1))
done

echo "
PeerOrgs:

  - Name: Org1
    Domain: org1.example.com
    Specs:" >> $file

i=0
for p in $peers ; do
echo "      - Hostname: peer$i
        SANS:
        - $p" >> $file
i=$((i+1))
done

echo "    Users:
      Count: 1" >> $file

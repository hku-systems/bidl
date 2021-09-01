#! /bin/bash
set -u

for i in `seq 20`; do
    echo "###  10.22.1.$i  ###"
	ssh-copy-id -i ~/.ssh/id_rsa.pub ${USER}@10.22.1.$i
done
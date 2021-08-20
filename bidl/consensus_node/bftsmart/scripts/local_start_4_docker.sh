#!/bin/bash

echo "Stop peers"
docker stop $(docker ps -aq --filter name=smart)
docker rm $(docker ps -aq --filter name=smart)
docker stop $(docker ps -aq --filter name=multicast)
docker rm $(docker ps -aq --filter name=multicast)

echo "Rebuilding image"
ant
cp ./scripts/configs/system_4.config ./config/system.config
rm -f config/currentView
rm -f smart.tar
rm -rf logs
docker image build -t smart .
mkdir logs

echo "Start 4 local nodes:"

docker run --name smart0 --net=HLF -p 11000:11000 -p 11001:11001 --cap-add NET_ADMIN smart bash /home/runscripts/smartrun.sh bftsmart.demo.microbenchmarks.ThroughputLatencyServer 0 10 0 0 false nosig rwd >logs/log_0.log 2>&1 &
docker run --name smart1 --net=HLF -p 11010:11010 -p 11011:11011 --cap-add NET_ADMIN smart bash /home/runscripts/smartrun.sh bftsmart.demo.microbenchmarks.ThroughputLatencyServer 1 10 0 0 false nosig rwd >logs/log_1.log 2>&1 &
docker run --name smart2 --net=HLF -p 11020:11020 -p 11021:11021 --cap-add NET_ADMIN smart bash /home/runscripts/smartrun.sh bftsmart.demo.microbenchmarks.ThroughputLatencyServer 2 10 0 0 false nosig rwd >logs/log_2.log 2>&1 &
docker run --name smart3 --net=HLF -p 11030:11030 -p 11031:11031 --cap-add NET_ADMIN smart bash /home/runscripts/smartrun.sh bftsmart.demo.microbenchmarks.ThroughputLatencyServer 3 10 0 0 false nosig rwd >logs/log_3.log 2>&1 &

# sleep 10

# echo "Add TC"
# docker exec smart0 iptables -I INPUT -p udp -m statistic --mode random --probability 0.05 -j DROP
# docker exec smart1 iptables -I INPUT -p udp -m statistic --mode random --probability 0.05 -j DROP
# docker exec smart2 iptables -I INPUT -p udp -m statistic --mode random --probability 0.05 -j DROP
# docker exec smart3 iptables -I INPUT -p udp -m statistic --mode random --probability 0.05 -j DROP

# docker exec smart0 tc qdisc add dev eth0 root netem loss 10%
# docker exec smart1 tc qdisc add dev eth0 root netem loss 10%
# docker exec smart2 tc qdisc add dev eth0 root netem loss 10%
# docker exec smart3 tc qdisc add dev eth0 root netem loss 10%
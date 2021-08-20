# Sequencer Instructions

# Workflow
1. Client sends messages to sequencer;
2. Sequencer inserts sequence number to messages, and broadcasts messages to 10.22.1.255:9999 (i.e., all servers in our cluster);
3. Servers receive ordered messages.

# Sequencer

## Compile Sequencer

```bash
# multicast sequencer, multicast messages to a group address
g++ -std=c++11 sequencer_multicast.cpp -o sequencer

# broadcast sequencer, broadcast messages to all receivers in the broadcast address
g++ -std=c++11 sequencer_broadcast.cpp -o sequencer
```

## Evaluate Sequencer Performance

1. Run Sequencer:

```bash
# bind with locahost:8888 
./sequencer > log 

# tps
grep "tps" log
```

2. Compile Server:

First set the `SERV_PORT` in the `udp_multicast_server.cpp` or
`udp_broadcast_server.cpp` files. 

```bash
# if you are running the multicast sequencer
g++ -std=c++11 udp_multicast_server.cpp -o server

# if you are running the broadcast sequencer
g++ -std=c++11 udp_broadcast_server.cpp -o server
```

3. Run Server:

```bash
./server
```

4. Compile and Run Client:

```bash
# suppose 10.22.1.8 is the sequencer address
g++ -std=c++11 udp_client.cpp -o client
./client 10.22.1.8

# read from file and repeatly send to sequencer 
go run udpClient.go --host 10.22.1.8 --file filename
```

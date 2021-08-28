# SBFT

# Run

1. Build base docker image: `docker build -f Dockerfile_base -t sbft:base .`
2. Run docker container: `docker run -it --rm -v $PWD:/home/SBFT sbft:base`
3. In docker container, build SBFT: 
    ```bash
    mkdir build 
    cd build

    # UDP
    cmake -DBUILD_ROCKSDB_STORAGE=TRUE -DCMAKE_CXX_FLAGS='-D SBFT_PAYLOAD_SIZE=32' ..  # set payload size

    ## TCP without TLS
    # cmake -DBUILD_ROCKSDB_STORAGE=TRUE -DBUILD_COMM_TCP_PLAIN=TRUE -DCMAKE_CXX_FLAGS='-D SBFT_PAYLOAD_SIZE=4096' ..  # set payload size

    ## TCP with TLS
    # cmake -DBUILD_ROCKSDB_STORAGE=TRUE -DBUILD_COMM_TCP_TLS=TRUE -DCMAKE_CXX_FLAGS='-D SBFT_PAYLOAD_SIZE=4096' ..  # set payload size

    make -j
    cd ..
    ```
4. Run: `bash evaluation/run.sh $clients $ops_per_client` 
5. Analyze log: `cat log_client* | python3 evaluation/tput.py`
6. Stop and clean: `bash evaluation/stop.sh`

# Evaluation

## UDP
|replicas|payloadsize(byte)|clients|tput|latency(ms)|
|---|---|---|---|---|
|4|0|1|140|6.97|
|4|0|10|755|13.088|
|4|0|100|6k|15.476|
|4|0|200|12k|18.699|
|4|0|300|15k|21.657|
|4|32|100|6.5k|16.142|
|4|32|200|10k|19.750|
|4|32|300|13k|24.375|

## TCP
|replicas|payloadsize(byte)|clients|tput|latency(ms)|
|---|---|---|---|---|
|4|0|100|1500|74.454|
|4|0|200|2800|75.809|
|4|0|300|4k|85.829|
|4|0|500|7k|87.528|
|4|0|1000|9k|125.398|
|4|1024|10|150|72.067|
|4|1024|50|800|64.852|
|4|1024|100|1500|69.931|
|4|1024|150|2000|75.593|
|4|4096|10|160|64.764|
|4|4096|50|700|70.085|
|4|4096|100|700|104.591|
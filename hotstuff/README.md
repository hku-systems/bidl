# HotStuff

# Run
1. Update host configuration in `docker/servers`.
2. Update user and password in `docker/config.sh`.
3. Build image and scp to other hosts: `cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED=ON -DHOTSTUFF_PROTO_LOG=ON;  make -j; bash docker/deploy.sh $tag`. 
4. Run servers: `bash docker/run.sh $image_tag`. 
5. Deploy: `bash run.sh setup`. (Set payload size in `scripts/deploy/group_vars/all.yml`)
6. Generate configuration: `bash gen_all.sh`.
7. Start Replicas: `bash run.sh new run1`.
8. Start clients: `bash run_cli.sh new run1_cli`.
9. Stop clients: `bash run_cli.sh stop run1_cli`.
10. Fetch log for analysis: `bash run_cli.sh fetch run1_cli`.
11. Analyze logs: `cat run1_cli/remote/*/log/stderr | python3 ../thr_hist.py --plot`.
12. Clean environment: `bash docker/kill_docker.sh`.


# Result

nodes = 4  
clients = 16  
batchsize = 400  
payload = [0, 128, 1024]  

## 0 byte



## 128 bytes


## 1024 bytes

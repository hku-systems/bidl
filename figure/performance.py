import numpy as np
import matplotlib.pyplot as plt
import sys

ff_tps = []
endorse_latency = []
commit_latency = []
with open("logs/ff/performance/log.log") as f: 
    for line in f.readlines():
        if line.startswith("ave"):
            temp = line.split()
            ff_tps.append(float(temp[-1]))
        if "endorse " in line:
            endorse_latency.append(float(line.split()[-1]))
        if "commit " in line:
            commit_latency.append(float(line.split()[-1]))


ff_latency = []
for i in range(0, 40, 5):
    temp = []
    for j in range(5): 
        idx = i + j
        temp.append(endorse_latency[idx] + commit_latency[idx])
    ff_latency.append(np.mean(temp))

fabric_tps = []
endorse_latency = []
commit_latency = []
with open("logs/fabric/log.log") as f: 
    for line in f.readlines():
        if line.startswith("ave"):
            temp = line.split()
            fabric_tps.append(float(temp[-1]))
        if "endorse " in line:
            endorse_latency.append(float(line.split()[-1]))
        if "commit " in line:
            commit_latency.append(float(line.split()[-1]))

fabric_latency = []
for i in range(0, len(endorse_latency), 5):
    temp = []
    for j in range(5): 
        idx = i + j
        temp.append(endorse_latency[idx] + commit_latency[idx])
    
    fabric_latency.append(np.mean(temp))

streamchain_latency = []
streamchain_tps = []
with open("logs/streamchain/validation.log") as f:
    for line in f.readlines():
        temp = line.split(',')
        streamchain_latency.append(float(temp[-2]))
        streamchain_tps.append(float(temp[-1]))

with open("logs/streamchain/endorsement.log") as f:
    cnt=0
    for line in f.readlines():
        temp = line.split(',')
        streamchain_latency[cnt] += float(temp[-2])
        cnt += 1

with open("logs/streamchain/ordering.log") as f:
    cnt=0
    for line in f.readlines():
        temp = line.split(',')
        streamchain_latency[cnt] += float(temp[-2])
        cnt += 1


# BIDL: TODO 

plt.plot(ff_tps, ff_latency, marker='x', label="fastfabric")
plt.plot(fabric_tps, fabric_latency, marker='o', label="fabric")
plt.plot(streamchain_tps, streamchain_latency, marker='s', label="streamchain")
# plt.plot(bidl_tps, bidl_latency, marker='P', label="BIDL")
plt.xlabel("throughput")
plt.ylabel("latency(ms)")
plt.legend()
plt.savefig("figure/performance.pdf")

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
        if "endorse" in line:
            endorse_latency.append(float(line.split()[-1]))
        if "commit" in line:
            commit_latency.append(float(line.split()[-1]))

ff_latency = []
for i in range(0, 40, 5):
    temp = []
    for j in range(5): 
        idx = i + j
        temp.append(endorse_latency[idx] + commit_latency[idx])
    ff_latency.append(np.mean(temp))


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


plt.plot(ff_tps, ff_latency, label="fastfabric")
plt.plot(streamchain_tps, streamchain_latency, label="streamchain")
plt.legend()
plt.savefig("figure/fig5.pdf")

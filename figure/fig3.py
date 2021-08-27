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
for i in range(0, len(endorse_latency), 5):
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
    print(temp)
plt.plot(ff_tps, ff_latency, label="fastfabric")
plt.plot(fabric_tps, fabric_latency, label="fabric")
plt.legend()
plt.savefig("figure/fig3.pdf")
